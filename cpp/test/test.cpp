#include <iostream>
#include <fstream>
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

const int SAMPLE_RATE = 44100;
const int CHANNELS = 2;
const enum AudioDataFormat PCM_FORMAT = PCM_FLOAT32;

bool WriteByteArrayToPcm(const char* filename, const char* data, size_t size) {
    ofstream file(filename, ios::binary | ios::ate);
    if (!file.is_open()) {
        cerr << "Unable to open file: " << filename << endl;
        return false;
    }

    file.write(data, size);
    return true;
}

char* ReadPcmToByteArray(const char* filename, size_t& size) {
    ifstream file(filename, ios::binary | ios::ate);
    if (!file.is_open()) {
        cerr << "Unable to open file: " << filename << endl;
        return nullptr;
    }

    size = file.tellg();
    file.seekg(0, ios::beg);

    char* data = new char[size];

    if (!file.read(data, size)) {
        cerr << "Error reading file: " << filename << endl;
        delete[] data;
        file.close();
        return nullptr;
    }

    file.close();
    return data;
}


int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <input_file_path> <vocal_model_path> <accompaniment_model_path>" << endl;
        return -1;
    }

    string input_file_path = argv[1];
    string vocal_model_path = argv[2];
    string accompaniment_model_path = argv[3];

    SignalInfo in_signal = {SAMPLE_RATE, CHANNELS, PCM_FORMAT};
    size_t in_size = 0;
    char* in = ReadPcmToByteArray(input_file_path.c_str(), in_size);

    double audio_duration = static_cast<double>(in_size) / in_signal.sample_rate;
    auto start_time = chrono::high_resolution_clock::now();

    bool isInit = true;
    Estimator es(vocal_model_path, accompaniment_model_path, in_signal, isInit);
    if (!isInit) {
        cout << red << "Failed to initialize Estimator" << reset << endl;
        return -1;
    }
    size_t num_frames = es.addFrames(in, in_size);
    if (num_frames != in_size / (in_signal.channels * sizeof(float))) {
        cout << red << "Failed to add frames" << reset << endl;
        return -1;
    }
    delete[] in; 

    char *out_1 = NULL;
    char *out_2 = NULL;
    size_t out_size = es.separate(&out_1, &out_2);

    auto end_time = chrono::high_resolution_clock::now();

    chrono::duration<double> inference_time = end_time - start_time;
    double real_time_factor = inference_time.count() / audio_duration;

    cout << yellow << "Inference time: " << inference_time.count() << " seconds" << reset << endl;
    cout << yellow << "Real-time factor: " << (int)round(1.0 / real_time_factor) << " : 1" << reset << endl;

    // Write separated channels to output files
    string output_1_name = "out_1.pcm";
    bool success = WriteByteArrayToPcm(output_1_name.c_str(), out_1, out_size);
    if (success) {
        cout << green << "Output written to " << output_1_name << reset << endl;
    } else {
        cerr << red << "Error writing output to " << output_1_name << reset << endl;
    }
    string output_2_name = "out_2.pcm";
    success = WriteByteArrayToPcm(output_2_name.c_str(), out_2, out_size);
    if (success) {
        cout << green << "Output written to " << output_2_name << reset << endl;
    } else {
        cerr << red << "Error writing output to " << output_2_name << reset << endl;
    }

    delete[] out_1;
    delete[] out_2;

    return 0;
}
