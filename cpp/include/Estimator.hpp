#ifndef Estimator_HPP
#define Estimator_HPP

#include "MNNDefine.h"
#include "Interpreter.hpp"
#include "Tensor.hpp"
#include <memory>
#include <vector>

class Estimator {
public:
    Estimator();
    std::vector<std::shared_ptr<MNN::Tensor>> separate(const std::vector<float>& stft_mag);

private:
    int F, T, win_length, hop_length;
    std::vector<std::shared_ptr<MNN::Interpreter>> interpreters;
    std::vector<MNN::Session*> sessions;
};

#endif // Estimator_HPP
