package android.hardware;

import android.util.Log;

public class Tvout {
    private static final String TAG = "Tvout_java";

    static {
        System.loadLibrary("tvout_jni");
    }

    public Tvout() {
        Log.i(TAG, "Tvout Initializing");
        _native_setup();
    }

    private native boolean _TvoutGetCableStatus();

    private native boolean _TvoutGetStatus();

    private native boolean _TvoutGetSubtitleStatus();

    private native boolean _TvoutGetSuspendStatus();

    private native boolean _TvoutPostSubtitle(String string, int param);

    private native boolean _TvoutPostSuspend(String string);

    private native boolean _TvoutSetCableStatus(boolean connected);

    private native boolean _TvoutSetOutputMode(int mode);

    private native boolean _TvoutSetResolution(int resolution);

    private native boolean _TvoutSetStatus(boolean enabled);

    private native boolean _TvoutSetSubtitleStatus(boolean enabled);

    private native boolean _TvoutSetSuspendStatus(boolean enabled);

    private native boolean _TvoutSetDefaultString(String string);

    private final native void _native_setup();

    private final native void _release();

    public boolean getCableStatus() {
        return _TvoutGetCableStatus();
    }

    public boolean getStatus() {
        return _TvoutGetStatus();
    }

    public boolean getSubtitleStatus() {
        return _TvoutGetSubtitleStatus();
    }

    public boolean getSuspendStatus() {
        return _TvoutGetSuspendStatus();
    }

    public boolean postSubtitle(String string, int paramInt) {
        return _TvoutPostSubtitle(string, paramInt);
    }

    public boolean postSuspend(String string) {
        return _TvoutPostSuspend(string);
    }

    public boolean setCableStatus(boolean connected) {
        return _TvoutSetCableStatus(connected);
    }

    public boolean setOutputMode(int mode) {
        return _TvoutSetOutputMode(mode);
    }

    public boolean setResolution(int resolution) {
        return _TvoutSetResolution(resolution);
    }

    public boolean setStatus(boolean enabled) {
        return _TvoutSetStatus(enabled);
    }

    public boolean setSubtitleStatus(boolean enabled) {
        return _TvoutSetSubtitleStatus(enabled);
    }

    public boolean setSuspendStatus(boolean enabled) {
        return _TvoutSetSuspendStatus(enabled);
    }

    public boolean setDefaultString(String string) {
        return _TvoutSetDefaultString(string);
    }

    public void release() {
        _release();
    }
}
