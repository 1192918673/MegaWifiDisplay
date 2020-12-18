package com.mega.wifidisplay;

public class WifiDisplaySource {
    public static native void setServer(String host, int port);

    public static native void setSize(int width, int height);

    public static native void start();

    public static native void stop();
}
