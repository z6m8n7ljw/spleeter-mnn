#ifndef UTILS_H_
#define UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <android/log.h>
#include <jni.h>

#define TAG "System.out"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, TAG, __VA_ARGS__)

#define SUCCESS 0
#define INVALID_OBJECT -100
#define NO_FOUND_FIELD_OBJECT -101

void jniThrowException(JNIEnv* env, const char* className, const char* msg);
void jniThrowIllegalArgumentException(JNIEnv* env, const char* msg);

#ifdef __cplusplus
}
#endif

#endif  // UTILS_H_
