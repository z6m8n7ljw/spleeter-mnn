package com.bilibili.audioseparation.audio;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Build;
import android.util.Log;

import androidx.annotation.RequiresApi;

import com.bilibili.audioseparation.utils.ArrayUtils;

import static android.media.AudioTrack.WRITE_BLOCKING;

public class AudioPlayer {
    private static final String TAG = "AudioPlayer";

    private static final int DEFAULT_STREAM_TYPE = AudioManager.STREAM_MUSIC;
    private static final int DEFAULT_SAMPLE_RATE = 44100;
    private static final int DEFAULT_CHANNEL_CONFIG = AudioFormat.CHANNEL_OUT_MONO;
    private static final int DEFAULT_AUDIO_FORMAT = AudioFormat.ENCODING_PCM_16BIT;
    private static final int DEFAULT_PLAY_MODE = AudioTrack.MODE_STREAM;

    private boolean mIsPlayStarted = false;
    private boolean mIsPlaying = false;
    private int mMinBufferSize = 0;
    private AudioTrack mAudioTrack;
    private int mAudioFormat = DEFAULT_AUDIO_FORMAT;

    public boolean startPlayer() {
        return startPlayer(DEFAULT_STREAM_TYPE, DEFAULT_SAMPLE_RATE, DEFAULT_CHANNEL_CONFIG, DEFAULT_AUDIO_FORMAT);
    }

    public boolean startPlayer(int sampleRateInHz, int channels, int audioFormat) {
        return startPlayer(DEFAULT_STREAM_TYPE, sampleRateInHz,
                channels >= 2 ? AudioFormat.CHANNEL_OUT_STEREO : AudioFormat.CHANNEL_OUT_MONO, audioFormat);
    }

    public boolean startPlayer(int streamType, int sampleRateInHz, int channelConfig, int audioFormat) {

        if (mIsPlayStarted) {
            Log.i(TAG, "Player already started !");
            return false;
        }

        mAudioFormat = audioFormat;
        mMinBufferSize = AudioTrack.getMinBufferSize(sampleRateInHz, channelConfig, audioFormat);
        if (mMinBufferSize == AudioTrack.ERROR_BAD_VALUE) {
            Log.e(TAG, "Invalid parameter !");
            return false;
        }
        Log.i(TAG, "getMinBufferSize = " + mMinBufferSize + " bytes !");

        mAudioTrack = new AudioTrack(streamType, sampleRateInHz, channelConfig, audioFormat, mMinBufferSize,
                DEFAULT_PLAY_MODE);
        if (mAudioTrack.getState() == AudioTrack.STATE_UNINITIALIZED) {
            Log.e(TAG, "AudioTrack initialize fail !");
            return false;
        }

        mIsPlayStarted = true;
        Log.i(TAG, "Start audio player success !");
        return true;
    }

    public boolean isPlayerStarted() {
        return mIsPlayStarted;
    }

    public int getMinBufferSize() {
        return mMinBufferSize;
    }

    public void stopPlayer() {
        if (!mIsPlayStarted) {
            return;
        }
        if (mAudioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
            mAudioTrack.stop();
        }
        mAudioTrack.release();
        mIsPlaying = false;
        mIsPlayStarted = false;

        Log.i(TAG, "Stop audio player success !");
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public boolean play(byte[] audioData, int sizeInBytes) {
        if (!mIsPlayStarted) {
            Log.e(TAG, "Player not started !");
            return false;
        }

        // if (sizeInBytes < mMinBufferSize) {
        // Log.w(TAG, "audio data is not enough maybe eof !!!!! ");
        // }

        int ret = 0;

        switch (mAudioFormat) {
        case AudioFormat.ENCODING_PCM_16BIT:
            ret = mAudioTrack.write(audioData, 0, sizeInBytes);
            break;
        case AudioFormat.ENCODING_PCM_FLOAT:
            float[] float_buffer = ArrayUtils.getFloatArrayInLittleOrder(audioData, 0, sizeInBytes);
            ret = mAudioTrack.write(float_buffer, 0, float_buffer.length, WRITE_BLOCKING);
            break;
        default:
            break;
        }

        if (ret < 0) {
            Log.e(TAG, "Could not write all the samples to the audio device(" + ret + ") !");
        }

        if (!mIsPlaying) {
            mAudioTrack.play();
            mIsPlaying = true;
        }
        return true;
    }
}
