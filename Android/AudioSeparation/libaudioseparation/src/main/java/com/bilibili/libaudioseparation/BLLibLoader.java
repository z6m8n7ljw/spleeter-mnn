package com.bilibili.libaudioseparation;

import android.text.TextUtils;

import java.io.File;

public class BLLibLoader {
    private static String libraryPath;

    /**
     * 传入动态库路径
     *
     * @param path: 动态库路径
     */
    public static void setLibraryPath(String path) {
        if (TextUtils.isEmpty(path)) {
            return;
        }
        if (path.endsWith(File.separator)) {
            libraryPath = path;
        } else {
            libraryPath = path + File.separator;
        }
    }

    /**
     * 加载动态库
     *
     * @param libraryName: 动态库名称
     */
    static void loadLibrary(String libraryName) {
        if (!TextUtils.isEmpty(libraryPath)) {
            System.load(libraryPath + "lib" + libraryName + ".so");
        } else {
            System.loadLibrary(libraryName);
        }
    }
}
