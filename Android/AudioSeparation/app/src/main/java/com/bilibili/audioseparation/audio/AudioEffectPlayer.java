package com.bilibili.audioseparation.audio;

import android.media.AudioFormat;
import android.os.Build;
import android.util.Log;

import androidx.annotation.RequiresApi;

import com.bilibili.audioseparation.utils.Utils;
import com.bilibili.libaudioseparation.BLAudioSeparation;

import java.io.IOException;
import java.io.OutputStream;
import java.io.RandomAccessFile;

public class AudioEffectPlayer implements Runnable {
    private static final String TAG = "AudioEffectPlayer";
    private int mSampleRateInHz = 44100;
    private int mChannels = 2;
    private int mAudioFormat = AudioFormat.ENCODING_PCM_FLOAT;

    private String mPcmFilePath = null;
    private AudioPlayer mPlayer;
    private boolean mIsPlaying = false;
    private BLAudioSeparation.BLAudioDataFormat mEffectDataFormat = BLAudioSeparation.BLAudioDataFormat.PCM_FLOAT32;

    public AudioEffectPlayer(int sampleRateInHz, int channels, int audioFormat) {
        mSampleRateInHz = sampleRateInHz;
        mChannels = channels;
        mAudioFormat = audioFormat;
    }

    public void setAudioEffectPlayer(String PcmFilePath) {
        mPcmFilePath = PcmFilePath;
    }

    public void stopAudioEffectPlayer() {
        mIsPlaying = false;
    }

    private void startAudioEffectPlayer() {
        switch (mAudioFormat) {
            case AudioFormat.ENCODING_PCM_16BIT:
                mEffectDataFormat = BLAudioSeparation.BLAudioDataFormat.PCM_INT16;
                break;
            case AudioFormat.ENCODING_PCM_FLOAT:
                mEffectDataFormat = BLAudioSeparation.BLAudioDataFormat.PCM_FLOAT32;
                break;
            default:
                break;
        }

        mPlayer = new AudioPlayer();
        mPlayer.startPlayer(mSampleRateInHz, mChannels, mAudioFormat);
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    @Override
    public void run() {
        try {
            // 打开播放文件
            RandomAccessFile inputRandomAccessFile = Utils.getInputRandomAccessFile(mPcmFilePath);
            // 创建并启动播放
            startAudioEffectPlayer();
            // 创建字节数组
            int bufferSize = mPlayer.getMinBufferSize();
            byte[] data = new byte[bufferSize];
            int ret = 0;
            // 循环读数据，播放音频
            mIsPlaying = true;
            while (mIsPlaying) {
                // 读取内容，放到字节数组里面
                int readSize = inputRandomAccessFile.read(data);
                if (readSize <= 0) {
                    Log.w(TAG, "  end of file stop player");
                    inputRandomAccessFile.seek(0);
                    continue;
                } else {
                    mPlayer.play(data, readSize);
                }
            }
            inputRandomAccessFile.close();
        } catch (IOException e) {
            e.printStackTrace();
        }

    }
}
