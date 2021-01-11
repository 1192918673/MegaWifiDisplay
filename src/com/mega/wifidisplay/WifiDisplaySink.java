package com.mega.wifidisplay;

import android.view.Surface;

public class WifiDisplaySink {
    public static native void setSurface(Surface surface);

    public static native void setVideoConfig(int width, int height, int fps,
            int profile, int level);

    public static native void setServer(String host, int port);

    public static native void start();

    public static native void stop();

    public static native void send(int type, float x, float y);
}
