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
import android.widget.Toast;
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

    private Thread mThread;
    private String mVocalModelFilePath = Utils.MODEL_FILE_VOCAL;
    private String mBgmModelFilePath = Utils.MODEL_FILE_BGM;
    private String mCurrentPcmFilePath = null;

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
    }


    @Override
    protected void onStop() {
        stopPlay();
//        mAudioSeparater.release();
        super.onStop();
    }

    private void startPlay() {
        mBtnPlay.setText("停止");
        mAudioPlayer = new AudioEffectPlayer(SAMPLE_RATE_IN_HZ, CHANNELS, DEFAULT_AUDIO_FORMAT);
        mAudioPlayer.setAudioEffectPlayer(mCurrentPcmFilePath);
        mThread = new Thread(mAudioPlayer);
        mThread.setPriority(Thread.MAX_PRIORITY);
        mThread.start();
    }

    private void stopPlay() {
        mBtnPlay.setText("试听");
        if (null != mThread && mThread.isAlive()) {
            mAudioPlayer.stopAudioEffectPlayer();
            mThread.interrupt();
            try {
                mThread.join();
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

    private void showToastCenteredWithCustomSize(String message) {
        Toast toast = new Toast(getApplicationContext());

        View toastView = getLayoutInflater().inflate(R.layout.custom_toast, null);
        TextView textView = toastView.findViewById(R.id.custom_toast_text);

        textView.setText(message);
        toast.setGravity(Gravity.CENTER_VERTICAL | Gravity.CENTER_HORIZONTAL, 0, 0);
        toast.setDuration(Toast.LENGTH_LONG);
        toast.setView(toastView);

        toast.show();
    }

    private void audioSeparate() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mProgressBar.setVisibility(View.VISIBLE);
            }
        });

        Thread audioSaparationThread = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    AudioSeparation audioSeparator = new AudioSeparation(mPcmFilePath, mVocalPcmFilePath, mBgmPcmFilePath, mVocalModelFilePath, mBgmModelFilePath, SAMPLE_RATE_IN_HZ, CHANNELS, DEFAULT_AUDIO_FORMAT);
                    audioSeparator.run();

                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            mProgressBar.setVisibility(View.GONE);
                            showToastCenteredWithCustomSize("音频分离完成");
                        }
                    });
                } catch (Exception e) {
                    Log.e("AudioSeparationActivity", "Error during audio separation", e);
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            mProgressBar.setVisibility(View.GONE);
                            showToastCenteredWithCustomSize("音频分离失败");
                        }
                    });
                }
            }
        });
        audioSaparationThread.start();
    }
}
