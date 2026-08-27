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

    // Legacy world-streaming and LiteRT native calls are not part of the canonical Android subset.
    // Keep the Java surface fail-closed until a separately audited native implementation exists.
    public static boolean startWorldStreaming(int seed, float sizeKm) {
        Log.w(TAG, "World streaming is unavailable in this debug artifact.");
        return false;
    }

    public static boolean updateCameraPosition(float x, float y, float z) {
        Log.w(TAG, "World streaming is unavailable in this debug artifact.");
        return false;
    }

    public static boolean stopWorldStreaming() {
        Log.w(TAG, "World streaming is unavailable in this debug artifact.");
        return false;
    }
}
