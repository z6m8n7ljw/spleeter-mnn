
#ifndef STFT_HPP
#define STFT_HPP

#include <Eigen/Dense>
#include <unsupported/Eigen/FFT>
#include <vector>
#include <cmath>

// Function to apply Hanning window
Eigen::VectorXf hanningWindow(int win_length) {
    Eigen::VectorXf window(win_length);
    for (int i = 0; i < win_length; ++i) {
        window(i) = 0.5 * (1 - cos(2 * M_PI * i / (win_length - 1)));
    }
    return window;
}

Eigen::VectorXf periodicHanningWindow(int win_length) {
    Eigen::VectorXf window(win_length);
    for (int i = 0; i < win_length; ++i) {
        window(i) = 0.5 * (1 - cos(2 * M_PI * i / win_length));
    }
    return window;
}

// Function to perform STFT
Eigen::MatrixXcf stft(const Eigen::VectorXf& signal, int n_fft, int hop_length, int win_length, const Eigen::VectorXf& win) {
    Eigen::FFT<float> fft;
    int half_n_fft = n_fft / 2;

    // Zero-padding
    Eigen::VectorXf padded_signal(signal.size() + n_fft);
    padded_signal << Eigen::VectorXf::Zero(half_n_fft), signal, Eigen::VectorXf::Zero(half_n_fft);

    // Calculate the number of frames
    int num_frames = 1 + (padded_signal.size() - n_fft) / hop_length;
    Eigen::MatrixXcf stft_matrix(n_fft / 2 + 1, num_frames);

    for (int i = 0; i < num_frames; ++i) {
        int start = i * hop_length;
        Eigen::VectorXf frame = padded_signal.segment(start, win_length).cwiseProduct(win);
        Eigen::VectorXcf frame_fft = fft.fwd(frame, n_fft);
        stft_matrix.col(i) = frame_fft.head(n_fft / 2 + 1);
    }

    return stft_matrix;
}

#endif // STFT_HPP