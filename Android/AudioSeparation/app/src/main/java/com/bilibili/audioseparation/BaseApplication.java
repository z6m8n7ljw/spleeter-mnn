package com.bilibili.audioseparation;

import android.annotation.SuppressLint;
import android.app.Application;
import android.content.Context;

public class BaseApplication extends Application {

    @SuppressLint("StaticFieldLeak")
    private static Context sContext;

    @Override
    public void onCreate() {
        super.onCreate();
        // if (LeakCanary.isInAnalyzerProcess(this)) {
        // return;
        // }
        // LeakCanary.install(this);
        sContext = getApplicationContext();
    }

    public static Context getContext() {
        return sContext;
    }
}
