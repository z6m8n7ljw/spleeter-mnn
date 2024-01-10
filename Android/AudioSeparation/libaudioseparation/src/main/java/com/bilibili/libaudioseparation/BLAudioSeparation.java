package com.bilibili.libaudioseparation;

import java.nio.ByteBuffer;

/**
 * {@link BLAudioSeparation} 类用于分离音频中的人声和背景声。
 */
public class BLAudioSeparation {
    public static final int INVALID_RESULT = -10; // 没有得到有效的返回值
    public static final int INVALID_OBJECT = -100; // 往native传入了无效的对象

    private final long mObject;

    static {
        BLLibLoader.loadLibrary("audio-separation-android");
    }

    /**
     * 音频分离异常定义，包含异常对应的 error message 和 error code.
     */
    public class BLAudioSeparationException extends Exception {
        private int errorCode;

        BLAudioSeparationException(String errMessage, int errCode) {
            super(errMessage);
            this.errorCode = errCode;
        }

        public int getErrorCode() {
            return errorCode;
        }
    }

    /**
     * 音频数据格式定义
     */

    public enum BLAudioDataFormat {
        /// 每个样本点由16位定点数表示
        PCM_INT16(0),
        /// 每个样本点由32位单精度浮点数表示
        PCM_FLOAT32(1);

        private int index;

        BLAudioDataFormat(int index) {
            this.index = index;
        }

        public int getIndex() {
            return index;
        }
    }

    /**
     * 音频分离引擎构造函数
     *
     * @param vocalModelPath         人声模型路径
     * @param accompanimentModelPath 背景声模型路径
     * @param sampleRate             音频数据采样率 e.g. {@code 44100}
     * @param channels               音频数据通道数
     * @param dataFormat             音频数据的数据类型 {@link BLAudioSeparation.BLAudioDataFormat}
     */
    public BLAudioSeparation(String vocalModelPath, String accompanimentModelPath, int sampleRate, int channels, BLAudioSeparation.BLAudioDataFormat dataFormat) {
        mObject = _getNativeBean(vocalModelPath, accompanimentModelPath, sampleRate, channels, dataFormat.getIndex());
    }


    /**
     * 向音频分离引擎添加音频帧
     *
     * @param input 音频数据
     * @param size  音频数据的字节数
     * @return 成功添加的采样点数
     */
    public int addFrames(byte[] input, int size) {
        return _addFrames(mObject, input, size);
    }

    /**
     * 音频分离
     *
     * @param out1 分离后的人声数据
     * @param out2 分离后的伴奏数据
     * @return 成功写入的字节数
     */
    public int separate(byte[] out1, byte[] out2) {
        return _separate(mObject, out1, out2);
    }

    /**
     * 释放音频分离引擎底层内存
     */
    public void release() {
        _releaseNativeBean(mObject);
    }

    // native methods
    private static native long _getNativeBean(int sampleRate, int channels, int dataFormat);

    private static native void _releaseNativeBean(long object);

    private static native int _addFrames(long object, byte[] input, int size);

    private static native int _separate(long object, byte[] out1, byte[] out2);
}
