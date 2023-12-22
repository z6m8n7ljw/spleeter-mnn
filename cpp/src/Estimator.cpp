#include "Estimator.hpp"
#include <iostream>

#define INPUT_NAME "onnx::Pad_0"
#define OUTPUT_NAME "379"

Estimator::Estimator() {
    // stft config
    F = 1024;
    T = 512;
    win_length = 4096;
    hop_length = 1024;

    // Load models and create sessions
    auto interpreter_vocal = std::shared_ptr<MNN::Interpreter>(MNN::Interpreter::createFromFile("../../models/vocal.mnn"));
    interpreters.push_back(interpreter_vocal);
    MNN::ScheduleConfig config_vocal;
    sessions.push_back(interpreter_vocal->createSession(config_vocal));

    auto interpreter_accompaniment = std::shared_ptr<MNN::Interpreter>(MNN::Interpreter::createFromFile("../../models/accompaniment.mnn"));
    interpreters.push_back(interpreter_accompaniment);
    MNN::ScheduleConfig config_accompaniment;
    sessions.push_back(interpreter_accompaniment->createSession(config_accompaniment));
}

std::vector<std::shared_ptr<MNN::Tensor>> Estimator::separate(const std::vector<float>& stft_mag) {
    std::vector<std::shared_ptr<MNN::Tensor>> masks;
    // Assume stft_mag is already in the correct shape (B, 2, 512, 1024)
    for (size_t i = 0; i < interpreters.size(); ++i) {
        auto interpreter = interpreters[i];
        auto session = sessions[i];
        auto input_tensor = interpreter->getSessionInput(session, INPUT_NAME);
        auto shape = input_tensor->shape();
        shape[0] = stft_mag.size() / (2 * 512 * 1024); // Update batch size
        interpreter->resizeTensor(input_tensor, shape);
        interpreter->resizeSession(session);
        auto output_tensor = interpreter->getSessionOutput(session, OUTPUT_NAME);

        // Create input tensor
        auto input_user = new MNN::Tensor(input_tensor, MNN::Tensor::CAFFE);
        memcpy(input_user->host<float>(), stft_mag.data(), input_user->size());
        input_tensor->copyFromHostTensor(input_user);
        delete input_user;

        // Run inference
        interpreter->runSession(session);

        // Fetch output tensor
        auto output_user = new MNN::Tensor(output_tensor, MNN::Tensor::CAFFE);
        output_tensor->copyToHostTensor(output_user);
        masks.push_back(std::shared_ptr<MNN::Tensor>(output_user));
    }
    return masks;
}
