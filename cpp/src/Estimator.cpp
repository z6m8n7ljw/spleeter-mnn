#include <iostream>
#include <stdexcept>
#include <functional>
#include "Estimator.hpp"
#include "Stft.hpp"

#define INPUT_NAME "onnx::Pad_0"
#define OUTPUT_NAME "379"

template<typename T, int NDIMS, int Options = Eigen::ColMajor>
static void print_helper(const Eigen::Tensor<T, NDIMS, Options>& tensor, const Eigen::Vector<long, NDIMS>& max_elements_per_dim) {
    auto dims = tensor.dimensions();
    std::cout << "Tensor dimensions: (";
    for (long i = 0; i < NDIMS; ++i) {
        std::cout << dims[i];
        if (i < NDIMS - 1) std::cout << ", ";
    }
    std::cout << ")" << std::endl;

    // Define a recursive function to traverse and print the values of the tensor
    std::function<void(long, Eigen::DSizes<Eigen::Index, NDIMS>&)> printValues;
    printValues = [&](long dim, Eigen::DSizes<Eigen::Index, NDIMS>& indices) {
        if (dim == NDIMS) {
            // Print the tensor value at the current index
            std::cout << "[";
            for (long i = 0; i < NDIMS; ++i) {
                std::cout << indices[i];
                if (i < NDIMS - 1) std::cout << ", ";
            }
            std::cout << "]: " << tensor(indices) << std::endl;
        } else {
            // Traverse the current dimension
            for (long i = 0; i < std::min<long>(dims[dim], max_elements_per_dim[dim]); ++i) {
                indices[dim] = i;
                printValues(dim + 1, indices);
            }
        }
    };

    // Start the recursive traversal
    Eigen::DSizes<Eigen::Index, NDIMS> indices;
    printValues(0, indices);
}

static Eigen::Tensor<float, 4, Eigen::RowMajor> pad_and_partition(const Eigen::Tensor<float, 4, Eigen::RowMajor>& array, int T) {
    int num_batches = array.dimension(0);
    int num_channels = array.dimension(1);
    int num_freq_bins = array.dimension(2);
    int L = array.dimension(3);

    int new_size = std::ceil(static_cast<float>(L) / T) * T;
    int pad_size = new_size - L;

    // Calculate the number of splits
    int split = new_size / T;

    // Create result tensor with the correct shape
    Eigen::Tensor<float, 4, Eigen::RowMajor> result(num_batches * split, num_channels, num_freq_bins, T);

    // If no padding is needed, we can directly partition the original array
    if (pad_size == 0) {
        for (int i = 0; i < split; ++i) {
            result.slice(Eigen::DSizes<Eigen::Index, 4>{i * num_batches, 0, 0, 0}, Eigen::DSizes<Eigen::Index, 4>{num_batches, num_channels, num_freq_bins, T}) = 
                array.slice(Eigen::DSizes<Eigen::Index, 4>{0, 0, 0, i * T}, Eigen::DSizes<Eigen::Index, 4>{num_batches, num_channels, num_freq_bins, T});
        }
    } else {
        // Pad the last dimension with zeros
        Eigen::array<std::pair<int, int>, 4> paddings = {std::make_pair(0, 0), std::make_pair(0, 0), std::make_pair(0, 0), std::make_pair(0, pad_size)};
        Eigen::Tensor<float, 4, Eigen::RowMajor> padded_array = array.pad(paddings);

        // Partition the padded tensor into segments of length T
        for (int i = 0; i < split; ++i) {
            result.slice(Eigen::DSizes<Eigen::Index, 4>{i * num_batches, 0, 0, 0}, Eigen::DSizes<Eigen::Index, 4>{num_batches, num_channels, num_freq_bins, T}) = 
                padded_array.slice(Eigen::DSizes<Eigen::Index, 4>{0, 0, 0, i * T}, Eigen::DSizes<Eigen::Index, 4>{num_batches, num_channels, num_freq_bins, T});
        }
    }

    return result;
}

Estimator::Estimator(const std::string& vocal_model_path, const std::string& accompaniment_model_path, const SignalInfo in_signal) : F(1024), T(512), win_length(4096), hop_length(1024) {
    this->win = periodicHanningWindow(this->win_length);
    this->signal_info = in_signal;

    MNN::Interpreter* interpreter = nullptr;
    MNN::Session* session = nullptr;

    // Load vocal model and create session
    interpreter = MNN::Interpreter::createFromFile(vocal_model_path.c_str());
    if (!interpreter) {
        throw std::runtime_error("Failed to load vocal model.");
    }
    this->interpreters.push_back(interpreter);
    MNN::ScheduleConfig config_vocal;
    session = interpreter->createSession(config_vocal);
    if (!session) {
        throw std::runtime_error("Failed to create session for vocal model.");
    }
    this->sessions.push_back(session);

    // Load accompaniment model and create session
    interpreter = MNN::Interpreter::createFromFile(accompaniment_model_path.c_str());
    if (!interpreter) {
        throw std::runtime_error("Failed to load accompaniment model.");
    }
    this->interpreters.push_back(interpreter);
    MNN::ScheduleConfig config_accompaniment;
    session = interpreter->createSession(config_accompaniment);
    if (!session) {
        throw std::runtime_error("Failed to create session for accompaniment model.");
    }
    this->sessions.push_back(session);
}

Estimator::~Estimator() {
    for (size_t i = 0; i < this->sessions.size(); ++i) {
        if (this->interpreters[i]) {
            this->interpreters[i]->releaseSession(this->sessions[i]);
            this->interpreters[i]->releaseModel();
            delete this->interpreters[i];
            this->interpreters[i] = nullptr;
        }
    }

    this->sessions.clear();
    this->interpreters.clear();
}

std::pair<Eigen::Tensor<float, 4, Eigen::RowMajor>, Eigen::Tensor<float, 3, Eigen::RowMajor>> Estimator::compute_stft(const Eigen::Tensor<float, 2, Eigen::RowMajor>& wav) {
    int num_channels = wav.dimension(0);
    int num_samples = wav.dimension(1);
    int num_frames = 1 + (num_samples + this->win_length - this->win_length) / this->hop_length;

    Eigen::Tensor<float, 4, Eigen::RowMajor> stft_stereo(num_channels, this->F, num_frames, 2);
    Eigen::Tensor<float, 3, Eigen::RowMajor> mag_stereo(num_channels, this->F, num_frames);

    for (int i = 0; i < num_channels; ++i) {
        Eigen::Map<const Eigen::VectorXf> vec(wav.data() + i * num_samples, num_samples);
        Eigen::MatrixXcf stft_mono = stft(vec, this->win_length, this->hop_length, this->win_length, this->win);

        for (int j = 0; j < num_frames; ++j) {
            for (int k = 0; k < this->F; ++k) {
                const auto& complex_val = stft_mono(k, j);
                stft_stereo(i, k, j, 0) = complex_val.real();
                stft_stereo(i, k, j, 1) = complex_val.imag();
                mag_stereo(i, k, j) = std::abs(complex_val);
            }
        }
    }

    return std::make_pair(stft_stereo, mag_stereo);
}

Eigen::Tensor<float, 2, Eigen::RowMajor> Estimator::compute_istft(const Eigen::Tensor<float, 4, Eigen::RowMajor>& stft) {
    // Create a tensor to hold the complex STFT values
    Eigen::Tensor<std::complex<float>, 3, Eigen::RowMajor> stft_complex(stft.dimension(0), stft.dimension(1), stft.dimension(2));

    // Extract real and imaginary parts and combine into a complex tensor
    stft_complex = stft.chip<3>(0).cast<std::complex<float>>() + 
                    stft.chip<3>(1).cast<std::complex<float>>() * std::complex<float>(0, 1);

    int pad = (win_length / 2 + 1) - F;
    assert(stft.dimension(1) + pad != this->win_length);
    Eigen::Tensor<std::complex<float>, 3, Eigen::RowMajor> padded_stft_complex(stft.dimension(0), stft.dimension(1) + pad, stft.dimension(2));
    padded_stft_complex.setZero();
    Eigen::DSizes<Eigen::Index, 3> offsets(0, 0, 0);
    Eigen::DSizes<Eigen::Index, 3> extents(stft.dimension(0), stft.dimension(1), stft.dimension(2));
    padded_stft_complex.slice(offsets, extents) = stft_complex;

    Eigen::FFT<float> fft;
    Eigen::Tensor<float, 3, Eigen::RowMajor> ifft_result(padded_stft_complex.dimension(0), this->win_length, padded_stft_complex.dimension(2));
    for (int b = 0; b < padded_stft_complex.dimension(0); ++b) {
        for (int t = 0; t < padded_stft_complex.dimension(2); ++t) {
            Eigen::VectorXcf slice(padded_stft_complex.dimension(1));
            for (int f = 0; f < padded_stft_complex.dimension(1); ++f) {
                slice(f) = padded_stft_complex(b, f, t);
            }

            Eigen::VectorXf ifft_vec(this->win_length);
            fft.inv(ifft_vec, slice, this->win_length);
            for (int w = 0; w < this->win_length; ++w) {
                ifft_result(b, w, t) = ifft_vec(w) * this->win(w);
            }
        }
    }

    int num_batches = ifft_result.dimension(0);
    int num_frames = ifft_result.dimension(2);
    int wav_length = this->win_length + (num_frames - 1) * this->hop_length;
    Eigen::Tensor<float, 2, Eigen::RowMajor> wavs(num_batches, wav_length);
    wavs.setZero();

    for (int b = 0; b < num_batches; ++b) {
        for (int t = 0; t < num_frames; ++t) {
            int start = t * this->hop_length;
            int end = start + this->win_length;
            for (int w = start; w < end; ++w) {
                wavs(b, w) += ifft_result(b, w - start, t);
            }
        }
    }

    return wavs;
}

size_t Estimator::addFrames(char *in, size_t byte_size) {
    if (this->signal_info.data_format == PCM_16BIT) {
        short *wav = reinterpret_cast<short*>(in);
        Eigen::Tensor<float, 2, Eigen::RowMajor> wav_float;
        size_t num_frames = byte_size / sizeof(short);
        size_t num_samples = num_frames / this->signal_info.channels;
        this->wav.resize(this->signal_info.channels, static_cast<long>(num_samples));
        wav_float.resize(this->signal_info.channels, static_cast<long>(num_samples));
        for (size_t i = 0; i < num_samples; ++i) {
            wav_float(0, i) = wav[this->signal_info.channels * i] / static_cast<float>(INT16_MAX);
            wav_float(1, i) = wav[this->signal_info.channels * i + 1] / static_cast<float>(INT16_MAX);
        }
        this->wav = wav_float;
    } else if (this->signal_info.data_format == PCM_FLOAT32) {
        float *wav = reinterpret_cast<float*>(in);
        size_t num_frames = byte_size / sizeof(float);
        size_t num_samples = num_frames / this->signal_info.channels;
        this->wav.resize(this->signal_info.channels, static_cast<long>(num_samples));
        for (size_t i = 0; i < num_samples; ++i) {
            this->wav(0, i) = wav[this->signal_info.channels * i];
            this->wav(1, i) = wav[this->signal_info.channels * i + 1];
        }
    }
    size_t wav_size = this->wav.dimension(1);
    byte_size = wav_size * this->signal_info.channels * (this->signal_info.data_format == PCM_16BIT ? sizeof(short) : sizeof(float));

    return byte_size;
}

size_t Estimator::separate(char *out_1, char *out_2) {
    auto stft_result = compute_stft(this->wav);
    auto stft = stft_result.first;
    auto stft_mag = stft_result.second;

    int L = stft.dimension(2);

    Eigen::DSizes<Eigen::Index, 4> broadcast_dims(1, 1, 1, 1);
    Eigen::Tensor<float, 4, Eigen::RowMajor> stft_mag_4d = stft_mag.reshape(Eigen::DSizes<Eigen::Index, 4>{stft_mag.dimension(0), stft_mag.dimension(1), stft_mag.dimension(2), 1}).broadcast(broadcast_dims);
    Eigen::DSizes<Eigen::Index, 4> shuffle_dims({3, 0, 1, 2});
    Eigen::Tensor<float, 4, Eigen::RowMajor> stft_mag_trans = stft_mag_4d.shuffle(shuffle_dims);
    Eigen::Tensor<float, 4, Eigen::RowMajor> stft_mag_padded = pad_and_partition(stft_mag_trans, this->T);
    shuffle_dims = {0, 1, 3, 2};
    Eigen::Tensor<float, 4, Eigen::RowMajor> stft_mag_trans_final = stft_mag_padded.shuffle(shuffle_dims);

    int B = stft_mag_trans_final.dimension(0);
    // Compute masks for each instrument using the neural network
    std::vector<Eigen::Tensor<float, 4, Eigen::RowMajor>> masks;
    for (size_t i = 0; i < this->interpreters.size(); ++i) {
        auto interpreter = this->interpreters[i];
        auto session = this->sessions[i];

        auto inputTensor = interpreter->getSessionInput(session, INPUT_NAME);
        interpreter->resizeTensor(inputTensor, {B, 2, this->T, this->F});
        interpreter->resizeSession(session);
        auto outputTensor = interpreter->getSessionOutput(session, OUTPUT_NAME);
        auto tmp_input = MNN::Tensor::create<float>({B, 2, this->T, this->F}, stft_mag_trans_final.data(), MNN::Tensor::CAFFE);
    
        inputTensor->copyFromHostTensor(tmp_input);
    
        interpreter->runSession(session);

        Eigen::Tensor<float, 4> zeros(B, 2, this->T, this->F);
        zeros.setZero();
        auto tmp_output = MNN::Tensor::create<float>({B, 2, this->T, this->F}, zeros.data(), MNN::Tensor::CAFFE);
        outputTensor->copyToHostTensor(tmp_output);
        Eigen::TensorMap<Eigen::Tensor<float, 4, Eigen::RowMajor>> tensorMap(tmp_output->host<float>(), B, 2, this->T, this->F);
        Eigen::Tensor<float, 4, Eigen::RowMajor> mask(B, 2, this->T, this->F);
        mask = tensorMap;
        masks.push_back(mask);

        delete tmp_input;
        delete tmp_output;
    }

    Eigen::Tensor<float, 4, Eigen::RowMajor> mask_sum = masks[0].square();
    mask_sum += masks[1].square();
    mask_sum = mask_sum + 1e-10f;

    std::vector<Eigen::Tensor<float, 2, Eigen::RowMajor>> wavs;
    for (auto& mask : masks) {
        mask = (mask.square() + (1e-10f / 2)) / mask_sum;

        Eigen::DSizes<Eigen::Index, 4> shuffle_dims(0, 1, 3, 2);
        Eigen::Tensor<float, 4, Eigen::RowMajor> mask_trans = mask.shuffle(shuffle_dims);

        std::vector<Eigen::Tensor<float, 4, Eigen::RowMajor>> splits;
        for (int i = 0; i < mask_trans.dimension(0); ++i) {
            Eigen::DSizes<Eigen::Index, 4> new_shape(1, mask_trans.dimension(1), mask_trans.dimension(2), mask_trans.dimension(3));
            splits.push_back(mask_trans.chip(i, 0).reshape(new_shape));
        }

        Eigen::Tensor<float, 4, Eigen::RowMajor> mask_concat(splits[0].dimension(0), splits[0].dimension(1), splits[0].dimension(2), B * splits[0].dimension(3));
        for (int i = 0; i < mask.dimension(0); ++i) {
            Eigen::DSizes<Eigen::Index, 4> offset(0, 0, 0, i * splits[0].dimension(3));
            mask_concat.slice(offset, splits[i].dimensions()) = splits[i];
        }

        Eigen::DSizes<Eigen::Index, 3> new_shape(mask_concat.dimension(1), mask_concat.dimension(2), mask_concat.dimension(3));
        Eigen::Tensor<float, 3, Eigen::RowMajor> mask_reshaped = mask_concat.reshape(new_shape);
        Eigen::DSizes<Eigen::Index, 3> offsets(0, 0, 0);
        Eigen::DSizes<Eigen::Index, 3> extents(mask_reshaped.dimension(0), mask_reshaped.dimension(1), L);
        Eigen::Tensor<float, 3, Eigen::RowMajor> mask_sliced = mask_reshaped.slice(offsets, extents);
        Eigen::DSizes<Eigen::Index, 4> new_dims(mask_sliced.dimension(0), mask_sliced.dimension(1), mask_sliced.dimension(2), 1);
        Eigen::Tensor<float, 4, Eigen::RowMajor> mask_expanded = mask_sliced.reshape(new_dims);

        Eigen::Tensor<float, 4, Eigen::RowMajor> stft_masked = stft * mask_expanded.broadcast(Eigen::DSizes<Eigen::Index, 4>{1, 1, 1, stft.dimension(3)});

        Eigen::Tensor<float, 2, Eigen::RowMajor> wav_masked = compute_istft(stft_masked);

        wavs.push_back(wav_masked);
    }

    size_t num_samples = wavs[0].dimension(1);
    size_t byte_size = num_samples * this->signal_info.channels * (this->signal_info.data_format == PCM_16BIT ? sizeof(short) : sizeof(float));
    if (this->signal_info.data_format == PCM_16BIT) {
        short *wav_1 = new short[this->signal_info.channels * num_samples];
        short *wav_2 = new short[this->signal_info.channels * num_samples];
        for (size_t j = 0; j < num_samples; ++j) {
            wav_1[this->signal_info.channels * j] = static_cast<short>(wavs[0](0, j) *  INT16_MAX);
            wav_1[this->signal_info.channels * j + 1] = static_cast<short>(wavs[0](1, j) *  INT16_MAX);
            wav_2[this->signal_info.channels * j] = static_cast<short>(wavs[1](0, j) *  INT16_MAX);
            wav_2[this->signal_info.channels * j + 1] = static_cast<short>(wavs[1](1, j) *  INT16_MAX);
        }
        memcpy(out_1, wav_1, byte_size);
        delete[] wav_1;
        memcpy(out_2, wav_2, byte_size);
        delete[] wav_2;
    } else if (this->signal_info.data_format == PCM_FLOAT32) {
        float *wav_1 = new float[this->signal_info.channels * num_samples];
        float *wav_2 = new float[this->signal_info.channels * num_samples];
        for (size_t j = 0; j < num_samples; ++j) {
            wav_1[this->signal_info.channels * j] = wavs[0](0, j);
            wav_1[this->signal_info.channels * j + 1] = wavs[0](1, j);
            wav_2[this->signal_info.channels * j] = wavs[1](0, j);
            wav_2[this->signal_info.channels * j + 1] = wavs[1](1, j);
        }
        memcpy(out_1, wav_1, byte_size);
        delete[] wav_1;
        memcpy(out_2, wav_2, byte_size);
        delete[] wav_2;
    }

    return byte_size;
}
