#include "utils.h"

void jniThrowException(JNIEnv* env, const char* className, const char* msg) {
    jclass newExcCls = (*env)->FindClass(env, className);
    if (!newExcCls) {
        LOGE("找不到异常类：%s", className);
        return;
    }
    if ((*env)->ThrowNew(env, newExcCls, msg) != JNI_OK) {
        LOGE("无法抛出异常:%s 描述:%s", className, msg);
        return;
    }
}

void jniThrowIllegalArgumentException(JNIEnv* env, const char* msg) {
    jniThrowException(env, "java/lang/IllegalArgumentException", msg);
}
