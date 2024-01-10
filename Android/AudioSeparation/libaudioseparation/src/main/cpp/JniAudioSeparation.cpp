#include <stdlib.h>
#include <stdexcept>
#include "lib-audio-separation/include/Estimator.h"
#include "utils.h"

/*
 * Class:     com_bilibili_libaudioseparation_BLAudioSeparation
 * Method:    _getNativeBean
 * Signature: (Ljava/lang/String;Ljava/lang/String;III)J
 */
JNIEXPORT jlong JNICALL
Java_com_bilibili_libaudioseparation_BLAudioSeparation__1getNativeBean(JNIEnv *env, jclass type, jstring vocalModelPath, jstring accompanimentModelPath, jint sampleRate, jint channels, jint dataFormat) {
    const char *vocal_model_path_ = (*env)->GetStringUTFChars(env, vocalModelPath, 0);
    const char *accompaniment_model_path_ = (*env)->GetStringUTFChars(env, accompaniment_model_path, 0);
    SignalInfo in_signal;
    in_signal.sample_rate = sampleRate;
    in_signal.channels = channels;
    in_signal.data_format = (enum AudioDataFormat)dataFormat;
    
    Estimator *estimator = nullptr;
    try {
        estimator = new Estimator(vocal_model_path_, accompaniment_model_path_, in_signal);
    } catch (const std::runtime_error &e) {
        LOGE("Failed to create Estimator: %s", e.what());
    }

    (*env)->ReleaseStringUTFChars(env, vocal_model_path, vocal_model_path_);
    (*env)->ReleaseStringUTFChars(env, accompaniment_model_path, accompaniment_model_path_);

    return (jlong)estimator;
}

/*
 * Class:     com_bilibili_libaudioseparation_BLAudioSeparation
 * Method:    _releaseNativeBean
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
Java_com_bilibili_libaudioseparation_BLAudioSeparation__1releaseNativeBean(JNIEnv *env, jclass type, jlong object) {
    Estimator *estimator = Estimator * object;
    if (estimator == nullptr) {
        LOGW("Invalid mObject Offsets. or may be died.");
        return;
    }
    delete estimator;
}

/*
 * Class:     com_bilibili_libaudioseparation_BLAudioSeparation
 * Method:    _addFrames
 * Signature: (J[BII)I
 */
JNIEXPORT jint JNICALL
Java_com_bilibili_libaudioseparation_BLAudioSeparation__1addFrames(JNIEnv *env, jclass type, jlong object, jbyteArray input, jint expectedBufferSize) {
    Estimator *estimator = Estimator * object;
    if (estimator == nullptr) {
        LOGW("Invalid Estimator object or may be died.");
        return INVALID_OBJECT;
    }

    jbyte *inputBuffer_ = (*env)->GetByteArrayElements(env, input, NULL);
    int result = estimator->addFrames(inputBuffer_, expectedBufferSize);
    (*env)->ReleaseByteArrayElements(env, input, inputBuffer_, 0);

    return result;
}

/*
 * Class:     com_bilibili_libaudioseparation_BLAudioSeparation
 * Method:    _separate
 * Signature: (J[B[B)I
 */
JNIEXPORT jint JNICALL
Java_com_bilibili_libaudioseparation_BLAudioSeparation__1separate(JNIEnv *env, jclass type, jlong object, jbyteArray output1, jbyteArray output2) {
    Estimator *estimator = Estimator * object;
    if (estimator == nullptr) {
        LOGW("Invalid Estimator object or may be died.");
        return INVALID_OBJECT;
    }

    jbyte *out_1_ = (*env)->GetByteArrayElements(env, output1, NULL);
    jbyte *out_2_ = (*env)->GetByteArrayElements(env, output2, NULL);

    int result = estimator->separate(&out_1_, &out_2_);

    (*env)->ReleaseByteArrayElements(env, output1, out_1_, 0);
    (*env)->ReleaseByteArrayElements(env, output2, out_2_, 0);

    return result;
}