#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
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

static bool WriteByteArrayToPcm(const char* filename, const char* data, size_t size) {
    ofstream file(filename, ios::binary | ios::ate);
    if (!file.is_open()) {
        cerr << "Unable to open file: " << filename << endl;
        return false;
    }

    file.write(data, size);
    return true;
}

static char* ReadPcmToByteArray(const char* filename, size_t& size) {
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

    // Read input file
    size_t byte_size = 0;
    char* in = ReadPcmToByteArray(input_file_path.c_str(), byte_size);
    if (in == nullptr) {
        cerr << red << "Error reading input file" << reset << endl;
        return -1;
    }
    auto start_time = chrono::high_resolution_clock::now();
    // Initialize Estimator
    Estimator *es = nullptr;
    SignalInfo in_signal = {SAMPLE_RATE, CHANNELS, PCM_FORMAT};
    try {
        es = new Estimator(vocal_model_path, accompaniment_model_path, in_signal);
    } catch (const runtime_error& e) {
        cerr << red << "Failed to initialize Estimator: " << e.what() << reset << endl;
        return -1;
    }

    // Separate channels
    size_t num_bytes = es->addFrames(in, byte_size);
    delete[] in; 

    char *out_vocal = new char[num_bytes + in_signal.sample_rate];
    char *out_bgm = new char[num_bytes + in_signal.sample_rate];
    byte_size = es->separate(out_vocal, out_bgm);

    delete es;
    auto end_time = chrono::high_resolution_clock::now();
    double audio_duration = static_cast<double>(num_bytes) / (in_signal.sample_rate * in_signal.channels * (PCM_FORMAT == PCM_FLOAT32 ? sizeof(float) : sizeof(short)));
    chrono::duration<double> inference_time = end_time - start_time;
    double real_time_factor = inference_time.count() / audio_duration;

    cout << yellow << "Inference time: " << inference_time.count() << " seconds" << reset << endl;
    cout << yellow << "Real-time factor: " << (int)round(1.0 / real_time_factor) << " : 1" << reset << endl;

    // Write separated channels to output files
    string vocal_file_path = "vocal.pcm";
    bool success = WriteByteArrayToPcm(vocal_file_path.c_str(), out_vocal, byte_size);
    if (success) {
        cout << green << "Output written to " << vocal_file_path << reset << endl;
    } else {
        cerr << red << "Error writing output to " << vocal_file_path << reset << endl;
    }
    string bgm_file_path = "bgm.pcm";
    success = WriteByteArrayToPcm(bgm_file_path.c_str(), out_bgm, byte_size);
    if (success) {
        cout << green << "Output written to " << bgm_file_path << reset << endl;
    } else {
        cerr << red << "Error writing output to " << bgm_file_path << reset << endl;
    }

    delete[] out_vocal;
    delete[] out_bgm;

    return 0;
}
