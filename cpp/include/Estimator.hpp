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

class Estimator {
public:
    Estimator(const std::string& vocal_model_path, const std::string& accompaniment_model_path, bool& success);
    ~Estimator();
    std::pair<Eigen::Tensor<float, 4, Eigen::RowMajor>, Eigen::Tensor<float, 3, Eigen::RowMajor>> compute_stft(const Eigen::Tensor<float, 2, Eigen::RowMajor>& wav);
    Eigen::Tensor<float, 2, Eigen::RowMajor> compute_istft(const Eigen::Tensor<float, 4, Eigen::RowMajor>& stft);
    std::vector<Eigen::Tensor<float, 2, Eigen::RowMajor>> separate(const Eigen::Tensor<float, 2, Eigen::RowMajor>& wav);
private:
    int F;
    int T;
    int win_length;
    int hop_length;
    Eigen::VectorXf win;
    std::vector<MNN::Interpreter *> interpreters;
    std::vector<MNN::Session *> sessions;
};

#endif // ESTIMATOR_HPP
