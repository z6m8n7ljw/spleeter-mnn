#include <stdlib.h>
#include <stdexcept>
#include "lib-audio-separation/include/Estimator.hpp"
#include "utils.h"

extern "C" {
/*
 * Class:     com_bilibili_libaudioseparation_BLAudioSeparation
 * Method:    _getNativeBean
 * Signature: (Ljava/lang/String;Ljava/lang/String;III)J
 */
JNIEXPORT jlong JNICALL
Java_com_bilibili_libaudioseparation_BLAudioSeparation__1getNativeBean(JNIEnv *env, jclass type,
                                                                       jstring vocalModelPath,
                                                                       jstring accompanimentModelPath,
                                                                       jint sampleRate,
                                                                       jint channels,
                                                                       jint dataFormat) {
    const char *vocal_model_path_ = env->GetStringUTFChars(vocalModelPath, 0);
    const char *accompaniment_model_path_ = env->GetStringUTFChars(accompanimentModelPath, 0);
    SignalInfo in_signal;
    in_signal.sample_rate = sampleRate;
    in_signal.channels = channels;
    in_signal.data_format = static_cast<AudioDataFormat>(dataFormat);

    Estimator *estimator = nullptr;
    try {
        estimator = new Estimator(vocal_model_path_, accompaniment_model_path_, in_signal);
    } catch (const std::runtime_error &e) {
        LOGE("Failed to create Estimator: %s", e.what());
    }

    env->ReleaseStringUTFChars(vocalModelPath, vocal_model_path_);
    env->ReleaseStringUTFChars(accompanimentModelPath, accompaniment_model_path_);

    return reinterpret_cast<jlong>(estimator);
}

/*
 * Class:     com_bilibili_libaudioseparation_BLAudioSeparation
 * Method:    _releaseNativeBean
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
Java_com_bilibili_libaudioseparation_BLAudioSeparation__1releaseNativeBean(JNIEnv *env, jclass type,
                                                                           jlong object) {
    Estimator *estimator = reinterpret_cast<Estimator *>(object);
    if (estimator == nullptr) {
        LOGW("Invalid Estimator object or may be died.");
        return;
    }
    delete estimator;
}

/*
 * Class:     com_bilibili_libaudioseparation_BLAudioSeparation
 * Method:    _addFrames
 * Signature: (J[BJ)I
 */
JNIEXPORT jint JNICALL
Java_com_bilibili_libaudioseparation_BLAudioSeparation__1addFrames(JNIEnv *env, jclass type,
                                                                   jlong object, jbyteArray input,
                                                                   jlong inputSize) {
    Estimator *estimator = reinterpret_cast<Estimator *>(object);
    if (estimator == nullptr) {
        LOGW("Invalid Estimator object or may be died.");
        return INVALID_OBJECT;
    }

    jbyte *inputBuffer_ = env->GetByteArrayElements(input, NULL);
    int result = estimator->addFrames(reinterpret_cast<char *>(inputBuffer_),
                                      static_cast<size_t>(inputSize));
    env->ReleaseByteArrayElements(input, inputBuffer_, 0);

    return result;
}

/*
 * Class:     com_bilibili_libaudioseparation_BLAudioSeparation
 * Method:    _separate
 * Signature: (J[B[B)I
 */
JNIEXPORT jint JNICALL
Java_com_bilibili_libaudioseparation_BLAudioSeparation__1separate(JNIEnv *env, jclass type,
                                                                  jlong object, jbyteArray output1,
                                                                  jbyteArray output2) {
    Estimator *estimator = reinterpret_cast<Estimator *>(object);
    if (estimator == nullptr) {
        LOGW("Invalid Estimator object or may be died.");
        return INVALID_OBJECT;
    }

    jbyte *out_1_ = env->GetByteArrayElements(output1, NULL);
    jbyte *out_2_ = env->GetByteArrayElements(output2, NULL);

    int result = estimator->separate(reinterpret_cast<char *>(out_1_),
                                     reinterpret_cast<char *>(out_2_));

    env->ReleaseByteArrayElements(output1, out_1_, 0);
    env->ReleaseByteArrayElements(output2, out_2_, 0);

    return result;
}
}