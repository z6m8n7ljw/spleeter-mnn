#include "Estimator.hpp"
#include <iostream>
#include <vector>

int main() {
    Estimator estimator;

    // Create dummy stft_mag data
    int B = 1; // Batch size
    int C = 2; // Channels
    int H = 512; // Height (frequency bins)
    int W = 1024; // Width (time frames)
    std::vector<float> dummy_stft_mag(B * C * H * W, 1.0f); // Fill with ones

    // Run separation
    auto masks = estimator.separate(dummy_stft_mag);
    std::cout << "Separation completed. Masks size: " << masks.size() << std::endl;

    return 0;
}
