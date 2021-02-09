package com.mega.wifidisplay;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.content.Intent;
import android.os.Bundle;
import android.os.Process;
import android.text.TextUtils;
import android.util.Log;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

import com.mega.wifidisplay.bluetooth.HidDataSender;
import com.mega.wifidisplay.input.TouchScreen.Report;

public class WifiDisplaySinkActivity extends Activity implements WfdConstants, SurfaceHolder.Callback {
    private static final String TAG = WifiDisplaySinkActivity.class.getSimpleName();

    private static final int SYSTEM_UI_IMMERSIVE_VISIBILITY =
            View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_LAYOUT_STABLE;

    private static final int SYSTEM_UI_FULLSCREEN_VISIBILITY =
            View.SYSTEM_UI_FLAG_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                    | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;

    private boolean mShowingNavBar;
    private SurfaceView mSurfaceView;
    private BluetoothAdapter   mAdapter;
    private HidDataSender mHidSender;
    private Report mReport;

    private final static byte REPORT_ID = 1;
    private final static String HID_NAME = "TouchScreen";
    private final static String HID_DESCRIPTION = "Mega Touch Screen";
    private final static String HID_PROVIDER = "Mega Touch Screen";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.sink_activity);

        showNavBar(false);
        mSurfaceView = findViewById(R.id.surface_view);
        mSurfaceView.getHolder().addCallback(this);
        mSurfaceView.getHolder().setKeepScreenOn(true);
        mSurfaceView.getHolder().setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);

        Intent intent = getIntent();
        String host = intent.getStringExtra(SOURCE_HOST);
        if (TextUtils.isEmpty(host)) {
            Log.w(TAG, "host is null or empty");
            finish();
            return;
        }
        int port = intent.getIntExtra(SOURCE_PORT, -1);
        if (port == -1) {
            Log.w(TAG, "port is -1");
            finish();
            return;
        }
        Log.d(TAG, "souce " + host + ':' + port);
        WifiDisplaySink.setServer(host, port);

        mAdapter = BluetoothAdapter.getDefaultAdapter();
        mReport = new Report();
        mHidSender = new HidDataSender(this,
                REPORT_ID,
                HID_NAME,
                HID_DESCRIPTION,
                HID_PROVIDER,
                (byte)0x05);
    }

    private void showNavBar(boolean show) {
        mShowingNavBar = show;
        if (show) {
            getWindow().getDecorView().setSystemUiVisibility(SYSTEM_UI_IMMERSIVE_VISIBILITY);
        } else {
            getWindow().getDecorView().setSystemUiVisibility(SYSTEM_UI_FULLSCREEN_VISIBILITY);
        }
    }

    //Car：1280 × 720
    private static final float RESOLUTION_INPUT_HOST_X = 1279;
    private static final float RESOLUTION_INPUT_HOST_Y = 719;

    //Pad：2560 × 1800  锤子：1080x2160
    //TODO：目前只能手动修改，后续需从底层获取对端的分辨率
    private static final float RESOLUTION_DEVICE_X = 1079;
    private static final float RESOLUTION_DEVICE_Y = 2159;

    private float RESOLUTION_SCALE_HEIGHT = (RESOLUTION_DEVICE_Y + 1) / (RESOLUTION_INPUT_HOST_Y + 1);
    private float SHADOW_IN_X = (RESOLUTION_INPUT_HOST_X - RESOLUTION_DEVICE_X / RESOLUTION_SCALE_HEIGHT) / 2;
    private float VIEWABLE_IN_X = SHADOW_IN_X + RESOLUTION_DEVICE_X / RESOLUTION_SCALE_HEIGHT;

    private float lastX = RESOLUTION_INPUT_HOST_X / 2;
    private float lastY = RESOLUTION_INPUT_HOST_Y / 2;
    private float curX = 0;
    private float curY = 0;

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        curX = event.getRawX();
        curY = event.getRawY();
        if (curX < SHADOW_IN_X)
            curX = (float) SHADOW_IN_X;
        else if (curX > VIEWABLE_IN_X)
            curX = (float) VIEWABLE_IN_X;
        final float x = (curX - lastX) * RESOLUTION_SCALE_HEIGHT;
        final float y = (curY - lastY) * RESOLUTION_SCALE_HEIGHT;
        //final float x = 20;
        //final float y = 20;
        Log.d(TAG, "ZZQ dispatchTouchEvent("+curX+","+curY+")"+",action:"+event.getAction()
                +",Relative offset("+x+","+y+")");

        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            mReport.setBtnTouch((byte)1, (int)x, (int)y);
        } else if (event.getAction() == MotionEvent.ACTION_MOVE) {
            mReport.setBtnTouch((byte)2, (int)x, (int)y);
        } else if (event.getAction() == MotionEvent.ACTION_UP) {
            mReport.setBtnTouch((byte)0, (int)x, (int)y);
        }

        mHidSender.sendReport(mReport, mReport.getReportQueue());
        if (!mReport.isBtnTouch()) {
            Log.d(TAG, "ZZQ dispatchTouchEvent()"+"curX:"+curX+",curY:"+curY);
            lastX = curX;
            lastY = curY;
        }
        return super.dispatchTouchEvent(event);
    }
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        /*Log.d(TAG, "onTouchEvent() action:" + event.getAction() + ", X:" + event.getX()
                + ", Y:" + event.getY());
        if (MotionEvent.ACTION_DOWN == event.getAction()) {

            showNavBar(!mShowingNavBar);
        }*/
        return super.onTouchEvent(event);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.d(TAG, "surfaceCreated");
        startSink(holder.getSurface());
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.d(TAG, "surfaceChanged format=" + format + " width=" + width + " height=" + height);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.d(TAG, "surfaceDestroyed");
        stopSink();
    }

    private void startSink(Surface surface) {
        Log.d(TAG, "startSink");
        WifiDisplaySink.setSurface(surface);
        WifiDisplaySink.setVideoConfig(mSurfaceView.getWidth(), mSurfaceView.getHeight(), 30, 1, 1);
        new Thread("WifiDisplaySinkThread") {
            @Override
            public void run() {
                Process.setThreadPriority(Process.THREAD_PRIORITY_DISPLAY);
                Log.d(TAG, getName() + " start");
                WifiDisplaySink.start();
                Log.d(TAG, getName() + " stop");
                stopSink();
            }
        }.start();
    }

    private void stopSink() {
        Log.d(TAG, "stopSink");
        WifiDisplaySink.stop();
        finish();
    }

    @Override
    protected void onStop() {
        super.onStop();
        Log.d(TAG, "onStop");
        stopSink();
    }
}
