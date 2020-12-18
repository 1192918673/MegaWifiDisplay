package com.mega.wifidisplay;

import android.app.Application;

public class WfdApplication extends Application {
    static {
        System.loadLibrary("jni_wfd");
    }
}
