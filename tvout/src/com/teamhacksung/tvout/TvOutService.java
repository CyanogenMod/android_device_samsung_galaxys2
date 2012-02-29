package com.teamhacksung.tvout;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.Tvout;
import android.nfc.Tag;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;

public class TvOutService extends Service {

    public static final String TAG = "TvOutService_java";

    private Tvout mTvOut;
    private boolean mWasOn = false; // For enabling on screen on

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (Intent.ACTION_HDMI_AUDIO_PLUG.equals(action)) {
                getTvoutInstance();
                int state = intent.getIntExtra("state", 0);
                if (state == 1 && !mTvOut.getStatus()) {
                    // Enable when cable is plugged
                    Log.i(TAG, "HDMI plugged");
                    mWasOn = false;
                    enable();
                } else if (mTvOut.getStatus()) {
                    // Disable when cable is unplugged
                    Log.i(TAG, "HDMI unplugged");
                    mWasOn = false;
                    disable();
                    releaseTvout();
                }
            } else if (Intent.ACTION_SCREEN_ON.equals(action)) {
                if (mTvOut != null && mWasOn) {
                    Log.i(TAG, "Screen On - Resume TvOut stream");
                    mWasOn = false;
                    mTvOut.setSuspendStatus(false);
                }
            } else if (Intent.ACTION_SCREEN_OFF.equals(action)) {
                if (mTvOut != null && mTvOut.getStatus()) {
                    Log.i(TAG, "Screen Off - Pausing TvOut stream");
                    mWasOn = true;
                    mTvOut.setSuspendStatus(true);
                }
            }
        }

    };

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        IntentFilter filter = new IntentFilter(Intent.ACTION_HDMI_AUDIO_PLUG);
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        filter.addAction(Intent.ACTION_SCREEN_ON);
        registerReceiver(mReceiver, filter);
        Log.i(TAG, "Registered Receiver");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return START_STICKY;
    }

    private boolean getTvoutInstance() {
        if (mTvOut != null) return true;

        try {
            mTvOut = new Tvout();
        } catch (Exception e) {
            return false;
        }

        return true;
    }

    private void releaseTvout() {
        if (mTvOut != null) {
            mTvOut.release();
            mTvOut = null;
        }
    }

    @Override
    public void onDestroy() {
        unregisterReceiver(mReceiver);
        releaseTvout();
        super.onDestroy();
    }

    private void enable() {
        if (mTvOut == null) return;
        mTvOut.setStatus(true);
        mTvOut.setCableStatus(true);
        mTvOut.setSuspendStatus(false);
    }

    private void disable() {
        if (mTvOut == null) return;
        mTvOut.setStatus(false);
        mTvOut.setCableStatus(false);
    }

}
