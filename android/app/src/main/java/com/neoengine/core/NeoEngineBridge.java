package com.neoengine.core;

import android.content.Context;
import android.webkit.JavascriptInterface;
import android.webkit.WebView;
import android.os.Build;
import android.util.Log;

public class NeoEngineBridge {

    private Context context;
    private WebView webView;
    private static final String TAG = "NeoEngineBridge";

    static {
        try { System.loadLibrary("neo_core"); }
        catch (UnsatisfiedLinkError e) { Log.w(TAG, "Native lib not found: " + e.getMessage()); }
    }

    public void init(Context ctx) {
        this.context = ctx;
        appContext = ctx.getApplicationContext();
    }

    public void setWebView(WebView wv) { this.webView = wv; }
    public void shutdown() { Log.d(TAG, "NeoEngineBridge shutdown"); }

    @JavascriptInterface
    public String getEngineVersion() { return "FauzanEngine v2.0"; }

    @JavascriptInterface
    public String getDeviceInfo() {
        return "{\"model\":\"" + Build.MODEL + "\",\"sdk\":" + Build.VERSION.SDK_INT + ",\"arch\":\"" + Build.SUPPORTED_ABIS[0] + "\"}";
    }

    @JavascriptInterface
    public void log(String message) { Log.d(TAG, message); }

    @JavascriptInterface
    public boolean isAndroid() { return true; }

    // ========== LiteRT availability gate ==========
    public static Context appContext;

    public static void initLiteRT(String modelPath) {
        Log.w(TAG, "LiteRT is unavailable in this debug artifact; local prompt execution is disabled.");
    }

    public static String sendPrompt(String prompt) {
        return "LITERT_UNAVAILABLE";
    }

    public static void shutdownLiteRT() {
        Log.d(TAG, "LiteRT remains unavailable in this debug artifact.");
    }

    // ========== Massive World Streaming ==========
    public static native void startWorldStreaming(int seed, float sizeKm);
    public static native void updateCameraPosition(float x, float y, float z);
    public static native void stopWorldStreaming();

    // Native callback from C++
    public static native void nativeOnLiteRTInitialized(boolean success);
}
