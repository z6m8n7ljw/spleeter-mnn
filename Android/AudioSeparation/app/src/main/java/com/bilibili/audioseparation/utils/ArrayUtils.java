package com.bilibili.audioseparation.utils;

import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import static android.content.ContentValues.TAG;

public class ArrayUtils {
    public static short[] toShortArray(byte[] src) {

        int count = src.length >> 1;
        short[] dest = new short[count];
        for (int i = 0; i < count; i++) {
            dest[i] = (short) (src[i * 2] << 8 | src[2 * i + 1] & 0xff);
        }
        return dest;
    }

    public static byte[] toByteArray(short[] src) {

        int count = src.length;
        byte[] dest = new byte[count << 1];
        for (int i = 0; i < count; i++) {
            dest[i * 2] = (byte) (src[i] >> 8);
            dest[i * 2 + 1] = (byte) (src[i] >> 0);
        }

        return dest;
    }

    public static short[] toShortArrayInBigEnd(byte[] b) {
        if (b == null || b.length % 2 != 0)
            throw new IllegalArgumentException("无法转换数组，输入参数错误：b == null or b.length % 2 != 0");

        short[] s = new short[b.length >> 1];
        ByteBuffer.wrap(b).order(ByteOrder.BIG_ENDIAN).asShortBuffer().get(s);
        return s;
    }

    public static short[] getShortArrayInLittleOrder(byte[] b, int offset, int length) {
        if (b == null || (length - offset) % 2 != 0)
            throw new IllegalArgumentException("无法转换数组，输入参数错误：b == null or b.length % 2 != 0");

        short[] s = new short[b.length >> 1];
        ByteBuffer.wrap(b, offset, length).order(ByteOrder.LITTLE_ENDIAN).asShortBuffer().get(s);
        return s;
    }

    public static short[] getShortArrayInLittleOrder(byte[] b) {
        if (b == null || b.length % 2 != 0)
            throw new IllegalArgumentException("无法转换数组，输入参数错误：b == null or b.length % 2 != 0");

        short[] s = new short[b.length >> 1];
        ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN).asShortBuffer().get(s);
        return s;
    }

    public static byte[] getByteArrayInLittleOrder(short[] shorts) {
        if (shorts == null) {
            return null;
        }
        byte[] bytes = new byte[shorts.length * 2];
        ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).asShortBuffer().put(shorts);
        return bytes;
    }

    public static byte[] getByteArrayInLittleOrder(float[] floats) {
        if (floats == null) {
            return null;
        }
        byte[] bytes = new byte[floats.length * 4];
        ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).asFloatBuffer().put(floats);
        return bytes;
    }

    public static float[] getFloatArrayInLittleOrder(byte[] b, int offset, int length) {
        if (b == null || 0 == length || (length - offset) % 4 != 0) {
            Log.w(TAG, "无法转换数组，输入参数错误：b == null or b.length % 4 != 0");
            return null;
        }

        float[] f = new float[length >> 2];
        ByteBuffer.wrap(b, offset, length).order(ByteOrder.LITTLE_ENDIAN).asFloatBuffer().get(f);
        return f;
    }

    public static float[] getFloatArrayInLittleOrder(byte[] b) {
        if (b == null || b.length % 4 != 0) {
            Log.w(TAG, "无法转换数组，输入参数错误：b == null or b.length % 4 != 0");
            return null;
        }

        float[] f = new float[b.length >> 2];
        ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN).asFloatBuffer().get(f);
        return f;
    }

    public static byte[] append(byte buffer1[], int length1, byte buffer2[], int length2) {
        byte[] bytes = new byte[length1 + length2];
        System.arraycopy(buffer1, 0, bytes, 0, length1);
        System.arraycopy(buffer2, 0, bytes, length1, length2);
        return bytes;
    }

}
