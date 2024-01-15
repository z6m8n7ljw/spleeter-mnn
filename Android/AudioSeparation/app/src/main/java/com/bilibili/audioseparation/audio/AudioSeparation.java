package com.bilibili.audioseparation.audio;

import android.media.AudioFormat;
import android.media.AudioTrack;
import android.os.Build;
import android.util.Log;

import androidx.annotation.RequiresApi;

import com.bilibili.audioseparation.utils.Utils;
import com.bilibili.libaudioseparation.BLAudioSeparation;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.io.FileOutputStream;

public class AudioSeparation implements Runnable {
    private static final String TAG = "AudioSeparation";
    private int mSampleRateInHz = 44100;
    private int mChannels = 2;
    private int mAudioFormat = AudioFormat.ENCODING_PCM_FLOAT;
    private BLAudioSeparation mAudioSeparation = null;
    
    private String mPcmFilePath;
    private String mVocalPcmFilePath;
    private String mBgmPcmFilePath;

    private String mVocalModelFilePath;
    private String mBgmModelFilePath;
    private BLAudioSeparation.BLAudioDataFormat mDataFormat = BLAudioSeparation.BLAudioDataFormat.PCM_FLOAT32;

    public AudioSeparation(String pcmFilePath, String vocalPcmFilePath, String bgmPcmFilePath, String vocalModelFilePath, String bgmModelFilePath, int sampleRateInHz, int channels, int audioFormat) {
        mPcmFilePath = pcmFilePath;
        mVocalPcmFilePath = vocalPcmFilePath;
        mBgmPcmFilePath = bgmPcmFilePath;
        mVocalModelFilePath = vocalModelFilePath;
        mBgmModelFilePath = bgmModelFilePath;
        mSampleRateInHz = sampleRateInHz;
        mChannels = channels;
        mAudioFormat = audioFormat;
        switch (mAudioFormat) {
            case AudioFormat.ENCODING_PCM_16BIT:
                mDataFormat = BLAudioSeparation.BLAudioDataFormat.PCM_INT16;
                break;
            case AudioFormat.ENCODING_PCM_FLOAT:
                mDataFormat = BLAudioSeparation.BLAudioDataFormat.PCM_FLOAT32;
                break;
            default:
                break;
        }
        mAudioSeparation = new BLAudioSeparation(mVocalModelFilePath, mBgmModelFilePath, mSampleRateInHz, mChannels, mDataFormat);
    }

    @Override
    public void run() {
        try (RandomAccessFile inputRandomAccessFile = Utils.getInputRandomAccessFile(mPcmFilePath)) {
            long fileLength = inputRandomAccessFile.length();
            Log.i(TAG, "getFileLength = " + fileLength + " bytes !");
            if (fileLength <= 0) {
                Log.w(TAG, "Empty file or end of file reached, stopping player");
                return;
            }
            byte[] data = new byte[(int) fileLength];
            inputRandomAccessFile.readFully(data);

            int ret = mAudioSeparation.addFrames(data, fileLength);
            if (ret < 0) {
                Log.e(TAG, "onAudioFrame: sendFrames");
                return;
            }
            byte[] vocalAudioData = new byte[ret + mSampleRateInHz * mChannels * 2];
            byte[] bgmAudioData = new byte[ret + mSampleRateInHz * mChannels * 2];
            ret = mAudioSeparation.separate(vocalAudioData, bgmAudioData);
            if (ret < 0) {
                Log.e(TAG, "Error separating audio data");
                return;
            }

            try (FileOutputStream vocalOut = new FileOutputStream(mVocalPcmFilePath)) {
                vocalOut.write(vocalAudioData, 0, ret);
            }
            try (FileOutputStream bgmOut = new FileOutputStream(mBgmPcmFilePath)) {
                bgmOut.write(bgmAudioData, 0, ret);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
