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

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << red << "Usage: " << argv[0] << " <input_file_path>" << reset << endl;
        return -1;
    }

    string input_file_path = argv[1];
    SF_INFO sfinfo;
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

    vector<float> interleaved_input(sfinfo.frames * sfinfo.channels);
    sf_count_t num_samples_read = sf_readf_float(infile, interleaved_input.data(), sfinfo.frames);
    sf_close(infile);

    if (num_samples_read != sfinfo.frames) {
        cerr << "Error reading audio data." << endl;
        return -1;
    }
    
    Tensor<float, 2, RowMajor> buffer(2, sfinfo.frames);
    for (long i = 0; i < sfinfo.frames; ++i) {
        buffer(0, i) = interleaved_input[2 * i];     // Left channel
        buffer(1, i) = interleaved_input[2 * i + 1]; // Right channel
    }

    cout << green << "Buffer Size: [" << buffer.dimension(0) << ", " << buffer.dimension(1) << "]" << reset << endl;
    double audio_duration = static_cast<double>(sfinfo.frames) / sfinfo.samplerate;

    auto start_time = chrono::high_resolution_clock::now();

    Estimator es;
    vector<Tensor<float, 2, RowMajor>> separated_channels = es.separate(buffer);

    auto end_time = chrono::high_resolution_clock::now();

    chrono::duration<double> inference_time = end_time - start_time;
    double real_time_factor = inference_time.count() / audio_duration;

    cout << green << "Separation completed." << reset << endl;
    cout << yellow << "Inference time: " << inference_time.count() << " seconds" << reset << endl;
    cout << yellow << "Real-time factor: " << (int)round(1.0 / real_time_factor) << " : 1" << reset << endl;

    // Write separated channels to output files
    for (size_t i = 0; i < separated_channels.size(); ++i) {
        string output_file_name = "out_" + to_string(i) + ".wav";
        SNDFILE* outfile = sf_open(output_file_name.c_str(), SFM_WRITE, &sfinfo);
        if (outfile == nullptr) {
            cerr << "Error opening output audio file." << endl;
            continue;
        }

        sf_count_t num_samples_write = separated_channels[i].dimension(1);
        vector<float> interleaved_output(num_samples_write * separated_channels[i].dimension(0));
        for (long j = 0; j < num_samples_write; ++j) {
            interleaved_output[2 * j] = separated_channels[i](0, j);     // Left channel
            interleaved_output[2 * j + 1] = separated_channels[i](1, j); // Right channel
        }

        if (sf_writef_float(outfile, interleaved_output.data(), num_samples_write) != num_samples_write) {
            cerr << "Error writing to output file." << endl;
        }

        sf_close(outfile);
        cout << green << "Output written to " << output_file_name << reset << endl;
    }

    return 0;
}
