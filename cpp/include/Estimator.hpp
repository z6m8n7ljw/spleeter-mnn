#ifndef ESTIMATOR_HPP
#define ESTIMATOR_HPP

#include <vector>
#include <string>
#include <cmath>
#include <complex>
#include "Eigen/Dense"
#include "unsupported/Eigen/CXX11/Tensor"
#include "MNN/MNNDefine.h"
#include "MNN/Interpreter.hpp"
#include "MNN/Tensor.hpp"

/**
 * @brief 音频数据格式
 *
 */
enum AudioDataFormat {
    PCM_16BIT = 0,  ///< 每个样本点由16位定点数表示
    PCM_FLOAT32 = 1 ///< 每个样本点由32位单精度浮点数表示
};

/**
 * @brief 音频数据参数信息
 *
 */
typedef struct SignalInfoT {
    int sample_rate;                  ///< 采样率
    uint8_t channels;                 ///< 声道数
    enum AudioDataFormat data_format; ///< 音频数据格式
} SignalInfo;

class Estimator {
public:
    Estimator(const std::string& vocal_model_path, const std::string& accompaniment_model_path, const SignalInfo in_signal);
    ~Estimator();
    std::pair<Eigen::Tensor<float, 4, Eigen::RowMajor>, Eigen::Tensor<float, 3, Eigen::RowMajor>> compute_stft(const Eigen::Tensor<float, 2, Eigen::RowMajor>& wav);
    Eigen::Tensor<float, 2, Eigen::RowMajor> compute_istft(const Eigen::Tensor<float, 4, Eigen::RowMajor>& stft);
    size_t addFrames(char *in, size_t size);
    size_t separate(char **out_1, char **out_2);
private:
    int F;
    int T;
    int win_length;
    int hop_length;
    Eigen::VectorXf win;
    SignalInfo signal_info;
    Eigen::Tensor<float, 2, Eigen::RowMajor> wav;
    std::vector<MNN::Interpreter *> interpreters;
    std::vector<MNN::Session *> sessions;
};

#endif // ESTIMATOR_HPP
