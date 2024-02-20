package com.bilibili.audioseparation.activity;

import android.media.AudioFormat;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.Gravity;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.graphics.Color;
import android.widget.ProgressBar;

import com.bilibili.audioseparation.BaseActivity;
import com.bilibili.audioseparation.R;
import com.bilibili.audioseparation.audio.AudioSeparation;
import com.bilibili.audioseparation.audio.AudioEffectPlayer;
import com.bilibili.audioseparation.utils.Utils;


public class AudioSeparationActivity extends BaseActivity implements View.OnClickListener, RadioGroup.OnCheckedChangeListener {
    private static final int SAMPLE_RATE_IN_HZ = 44100;
    private static final int CHANNELS = 2;
    private static final int DEFAULT_AUDIO_FORMAT = AudioFormat.ENCODING_PCM_FLOAT;
    private AudioEffectPlayer mAudioPlayer;
    private Button mBtnPlay;
    private Button mBtnAudioSeparate;
    private ProgressBar mProgressBar;
    private TextView mStatusText;
    private TextView mDurationText;
    private TextView mProcessingTimeText;

    private Thread mThreadPlay;
    private Thread mThreadSep;
    private String mVocalModelFilePath = Utils.MODEL_FILE_VOCAL;
    private String mBgmModelFilePath = Utils.MODEL_FILE_BGM;
    private String mCurrentPcmFilePath = Utils.PCM_FILE_INPUT;

    private String mPcmFilePath = Utils.PCM_FILE_INPUT;
    private String mVocalPcmFilePath = Utils.PCM_FILE_VOCAL;
    private String mBgmPcmFilePath = Utils.PCM_FILE_BGM;

    private boolean isClearingCheck = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_audioseparation);

        mBtnAudioSeparate = findViewById(R.id.btn_audio_separate);
        mBtnAudioSeparate.setOnClickListener(this);

        mProgressBar = findViewById(R.id.progress_bar);

        mBtnPlay = findViewById(R.id.btn_play);
        mBtnPlay.setOnClickListener(this);

        ImageButton backButton = findViewById(R.id.back_button);
        backButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                finish();
            }
        });

        ((RadioGroup) findViewById(R.id.file_in_group)).setOnCheckedChangeListener(this);
        ((RadioGroup) findViewById(R.id.file_out_group)).setOnCheckedChangeListener(this);

        mStatusText = findViewById(R.id.status_text);
        mDurationText = findViewById(R.id.duration_text);
        mProcessingTimeText = findViewById(R.id.processing_time_text);

        mStatusText.setTextColor(Color.RED);
        mStatusText.setText("未完成");
        mDurationText.setText("");
        mProcessingTimeText.setText("");
    }

    private void startPlay() {
        mBtnPlay.setText("停止");
        mAudioPlayer = new AudioEffectPlayer(SAMPLE_RATE_IN_HZ, CHANNELS, DEFAULT_AUDIO_FORMAT);
        mAudioPlayer.setAudioEffectPlayer(mCurrentPcmFilePath);
        mThreadPlay = new Thread(mAudioPlayer);
        mThreadPlay.setPriority(Thread.MAX_PRIORITY);
        mThreadPlay.start();
    }

    private void stopPlay() {
        mBtnPlay.setText("试听");
        if (null != mThreadPlay && mThreadPlay.isAlive()) {
            mAudioPlayer.stopAudioEffectPlayer();
            mThreadPlay.interrupt();
            try {
                mThreadPlay.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    @Override
    public void onClick(View v) {
        int id = v.getId();
        if (id == R.id.btn_play) {
            if (mBtnPlay.getText().toString().contentEquals("试听")) {
                startPlay();
            } else if (mBtnPlay.getText().toString().contentEquals("停止")) {
                stopPlay();
            }
        } else if (id == R.id.btn_audio_separate) {
            audioSeparate();
        }
    }

    @Override
    public void onCheckedChanged(RadioGroup group, int checkedId) {
        if (isClearingCheck) {
            return;
        }

        RadioGroup group1 = findViewById(R.id.file_in_group);
        RadioGroup group2 = findViewById(R.id.file_out_group);

        isClearingCheck = true;
        if (group == group1) {
            group2.clearCheck();
        } else if (group == group2) {
            group1.clearCheck();
        }
        isClearingCheck = false;

        switch (checkedId) {
            case R.id.rb_pcm_input:
                mCurrentPcmFilePath = mPcmFilePath;
                break;
            case R.id.rb_pcm_vocal:
                mCurrentPcmFilePath = mVocalPcmFilePath;
                break;
            case R.id.rb_pcm_bgm:
                mCurrentPcmFilePath = mBgmPcmFilePath;
                break;
            default:
                break;
        }
    }

    private void audioSeparate() {
        if (mThreadSep != null && mThreadSep.isAlive()) {
            return;
        }
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mProgressBar.setVisibility(View.VISIBLE);
            }
        });

        mThreadSep = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    final long mStartTime = System.currentTimeMillis();
                    AudioSeparation audioSeparator = new AudioSeparation(mPcmFilePath, mVocalPcmFilePath, mBgmPcmFilePath, mVocalModelFilePath, mBgmModelFilePath, SAMPLE_RATE_IN_HZ, CHANNELS, DEFAULT_AUDIO_FORMAT);
                    audioSeparator.run();
                    final float processingTime = (System.currentTimeMillis() - mStartTime) / 1000.0f;

                    final float duration = audioSeparator.getTimeDuration();

                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            mProgressBar.setVisibility(View.GONE);
                            mStatusText.setTextColor(Color.GREEN);
                            mStatusText.setText("已完成");
                            mDurationText.setText(String.format("%.2f 秒", duration));
                            mProcessingTimeText.setText(String.format("%.2f 秒", processingTime));
                        }
                    });
                } catch (Exception e) {
                    Log.e("AudioSeparationActivity", "Error during audio separation", e);
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            mProgressBar.setVisibility(View.GONE);
                        }
                    });
                } finally {
                    mThreadSep = null;
                }
            }
        });
        mThreadSep.start();
    }
}
