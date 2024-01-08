#include <iostream>
#include <iomanip>
#include <chrono>
#include <sndfile.h>
#include "Estimator.hpp"

// Define ANSI color codes
const char* red = "\033[31m";
const char* green = "\033[32m";
const char* yellow = "\033[33m";
const char* reset = "\033[0m";

using namespace std;
using namespace Eigen;

enum AudioDataFormat {
    PCM_16BIT = 0,  ///< 每个样本点由16位定点数表示
    PCM_FLOAT32 = 1 ///< 每个样本点由32位单精度浮点数表示
};

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <input_file_path> <vocal_model_path> <accompaniment_model_path>" << std::endl;
        return -1;
    }

    string input_file_path = argv[1];
    std::string vocal_model_path = argv[2];
    std::string accompaniment_model_path = argv[3];

    SF_INFO sfinfo;
    sfinfo.format = SF_FORMAT_RAW | SF_FORMAT_FLOAT; // SF_FORMAT_PCM_16;
    sfinfo.channels = 2;
    sfinfo.samplerate = 44100;
    SNDFILE* infile = sf_open(input_file_path.c_str(), SFM_READ, &sfinfo);
    if (infile == nullptr) {
        cerr << red << "Error opening input file." << reset << endl;
        return -1;
    }

    cout << green << "Sample Rate: " << sfinfo.samplerate << " Hz" << reset << endl;
    cout << green << "Channels: " << sfinfo.channels << reset << endl;
    if (sfinfo.channels != 2) {
        cerr << red << "Error: Only stereo audio supported." << reset << endl;
        sf_close(infile);
        return -1;
    }

    AudioDataFormat format;
    switch (sfinfo.format & SF_FORMAT_SUBMASK) {
        case SF_FORMAT_PCM_16:
            format = PCM_16BIT;
            cout << green << "Format: INT 16" << reset << endl;
            break;
        case SF_FORMAT_FLOAT:
            format = PCM_FLOAT32;
            cout << green << "Format: FLOAT 32" << reset << endl;
            break;
        default:
            cerr << "Unsupported audio format." << endl;
            sf_close(infile);
            return -1;
    }

    Tensor<float, 2, RowMajor> in_buffer(2, sfinfo.frames);
    if (format == PCM_16BIT) {
        vector<short> interleaved_input(sfinfo.frames * sfinfo.channels);
        sf_count_t num_samples_read = sf_readf_short(infile, interleaved_input.data(), sfinfo.frames);
        if (num_samples_read != sfinfo.frames) {
            cerr << red << "Error reading audio data."  << reset << endl;
            sf_close(infile);
            return -1;
        }
        for (long i = 0; i < sfinfo.frames; ++i) {
            in_buffer(0, i) = interleaved_input[2 * i] / static_cast<float>(INT16_MAX);
            in_buffer(1, i) = interleaved_input[2 * i + 1] / static_cast<float>(INT16_MAX);
        }
    } else if (format == PCM_FLOAT32) {
        vector<float> interleaved_input(sfinfo.frames * sfinfo.channels);
        sf_count_t num_samples_read = sf_readf_float(infile, interleaved_input.data(), sfinfo.frames);
        if (num_samples_read != sfinfo.frames) {
            cerr << red << "Error reading audio data." << reset << endl;
            sf_close(infile);
            return -1;
        }        
        for (long i = 0; i < sfinfo.frames; ++i) {
            in_buffer(0, i) = interleaved_input[2 * i];
            in_buffer(1, i) = interleaved_input[2 * i + 1];
        }
    }
    sf_close(infile);

    cout << green << "Buffer Size: [" << in_buffer.dimension(0) << ", " << in_buffer.dimension(1) << "]" << reset << endl;

    double audio_duration = static_cast<double>(sfinfo.frames) / sfinfo.samplerate;
    auto start_time = chrono::high_resolution_clock::now();

    bool isInit = true;
    Estimator es(vocal_model_path, accompaniment_model_path, isInit);
    if (!isInit) {
        cout << red << "Failed to initialize Estimator" << reset << endl;
        return -1;
    }
    vector<Tensor<float, 2, RowMajor>> out_buffer = es.separate(in_buffer);

    auto end_time = chrono::high_resolution_clock::now();

    chrono::duration<double> inference_time = end_time - start_time;
    double real_time_factor = inference_time.count() / audio_duration;

    cout << yellow << "Inference time: " << inference_time.count() << " seconds" << reset << endl;
    cout << yellow << "Real-time factor: " << (int)round(1.0 / real_time_factor) << " : 1" << reset << endl;

    // Write separated channels to output files
    for (size_t i = 0; i < out_buffer.size(); ++i) {
        string output_file_name = "out_" + to_string(i) + ".pcm";
        SNDFILE* outfile = sf_open(output_file_name.c_str(), SFM_WRITE, &sfinfo);
        if (outfile == nullptr) {
            cerr << "Error opening output audio file." << endl;
            continue;
        }

        sf_count_t num_samples_write = out_buffer[i].dimension(1);
        if (format == PCM_16BIT) {
            vector<short> interleaved_output(num_samples_write * out_buffer[i].dimension(0));
            for (long j = 0; j < num_samples_write; ++j) {
                interleaved_output[2 * j] = static_cast<short>(out_buffer[i](0, j) *  INT16_MAX);
                interleaved_output[2 * j + 1] = static_cast<short>(out_buffer[i](1, j) *  INT16_MAX);
            }

            if (sf_writef_short(outfile, interleaved_output.data(), num_samples_write) != num_samples_write) {
                cerr << "Error writing to output file." << endl;
            }
        } else if (format == PCM_FLOAT32) {
            vector<float> interleaved_output(num_samples_write * out_buffer[i].dimension(0));
            for (long j = 0; j < num_samples_write; ++j) {
                interleaved_output[2 * j] = out_buffer[i](0, j); 
                interleaved_output[2 * j + 1] = out_buffer[i](1, j);
            }

            if (sf_writef_float(outfile, interleaved_output.data(), num_samples_write) != num_samples_write) {
                cerr << "Error writing to output file." << endl;
            }
        }

        sf_close(outfile);
        cout << green << "Output written to " << output_file_name << reset << endl;
    }

    return 0;
}
