package com.bilibili.audioseparation.utils;

import android.content.Context;
import android.content.res.AssetManager;
import android.os.Handler;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.util.Log;

import com.bilibili.audioseparation.BaseApplication;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.RandomAccessFile;

public class Utils {

    private static final String SDCARD = "/sdcard/";// Environment.getExternalStorageDirectory().getAbsolutePath();
    private static final String APP_NAME = "audio_effect/";
    public static final String APP_DIR = SDCARD + APP_NAME;
    public static final String PCM_FILE_EFFECT = APP_DIR + "effect.pcm";
    public static final String PCM_FILE_INPUT = APP_DIR + "coc.pcm";
    public static final String PCM_FILE_VOCAL = APP_DIR + "vocal.pcm";
    public static final String PCM_FILE_BGM = APP_DIR + "bgm.pcm";
    public static final String MODEL_FILE_VOCAL = APP_DIR + "vocal.mnn";
    public static final String MODEL_FILE_BGM = APP_DIR + "accompaniment.mnn";

    private static Handler sHandler = new Handler(Looper.getMainLooper());

    static {
        createDir(APP_DIR);
    }

    private static void createDir(String path) {
        File file = new File(path);
        if (!file.exists() && !file.mkdir()) {
            ToastHelper.show("文件夹创建过程中出现错误: " + path);
        }
    }

    public static void copyFromAssets(Context context, String oldFilePath, String newFilePath) {
        try {
            InputStream is = context.getAssets().open(oldFilePath);
            FileOutputStream fos = new FileOutputStream(new File(newFilePath));
            byte[] buffer = new byte[1024];
            int byteCount = 0;
            while ((byteCount = is.read(buffer)) != -1) {// 循环从输入流读取 buffer字节
                fos.write(buffer, 0, byteCount);// 将读取的输入流写入到输出流
            }
            fos.flush();// 刷新缓冲区
            is.close();
            fos.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static String getFromAssets(Context context, String fileName) {
        String result = "";
        try {
            InputStream in = context.getAssets().open(fileName);
            // 获取文件的字节数
            int lenght = in.available();
            // 创建byte数组
            byte[] buffer = new byte[lenght];
            // 将文件中的数据读到byte数组中
            in.read(buffer);
            result = new String(buffer);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return result;
    }

    public static void runOnUiThread(Runnable action) {
        if (Looper.myLooper() == Looper.getMainLooper()) {
            action.run();
        } else {
            sHandler.post(action);
        }
    }

    public static RandomAccessFile getInputRandomAccessFile(String filePath) {
        RandomAccessFile result = null;
        File file = new File(filePath);
        try {
            result = new RandomAccessFile(file,"r");
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }
        return result;
    }

    public static InputStream getInputStream(String filePath) {
        InputStream result = null;
        File file = new File(filePath);
        try {
            result = new FileInputStream(file);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }
        return result;
    }

    public static OutputStream getOutputStream(String filePath) {
        OutputStream result = null;
        File file = new File(filePath);
        if (file.exists()) {
            file.delete();
        }
        try {
            result = new FileOutputStream(file);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }
        return result;
    }

    public static void saveAsFile(String content, String filePath) {
        if (null == content) return;
        FileWriter fwriter = null;
        File file = new File(filePath);
        if (file.exists()) {
            file.delete();
        }
        try {
            // true表示不覆盖原来的内容，而是加到文件的后面。若要覆盖原来的内容，直接省略这个参数就好
            fwriter = new FileWriter(filePath, true);
            fwriter.write(content);
        } catch (IOException ex) {
            ex.printStackTrace();
        } finally {
            try {
                fwriter.flush();
                fwriter.close();
            } catch (IOException ex) {
                ex.printStackTrace();
            }
        }
    }

    public static int getScreenWidth() {
        DisplayMetrics metrics = BaseApplication.getContext().getResources().getDisplayMetrics();
        return metrics.widthPixels;
    }

    public static int getScreenHeight() {
        DisplayMetrics metrics = BaseApplication.getContext().getResources().getDisplayMetrics();
        return metrics.heightPixels;
    }

}
