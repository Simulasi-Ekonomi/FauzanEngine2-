package com.neoengine.core;

import android.content.Context;
import android.webkit.JavascriptInterface;
import android.webkit.WebView;
import android.os.Build;
import android.util.Log;
import kotlinx.coroutines.BuildersKt;
import kotlinx.coroutines.Dispatchers;

public class NeoEngineBridge {

    private Context context;
    private WebView webView;
    private static final String TAG = "NeoEngineBridge";

    static {
        try {
            System.loadLibrary("neo_core");
        } catch (UnsatisfiedLinkError e) {
            Log.w(TAG, "Native lib not found: " + e.getMessage());
        }
    }

    public void init(Context ctx) {
        this.context = ctx;
        appContext = ctx.getApplicationContext();
    }

    public void setWebView(WebView wv) {
        this.webView = wv;
    }

    public void shutdown() {
        Log.d(TAG, "NeoEngineBridge shutdown");
    }

    @JavascriptInterface
    public String getEngineVersion() {
        return "FauzanEngine v2.0";
    }

    @JavascriptInterface
    public String getDeviceInfo() {
        return "{\"model\":\"" + Build.MODEL +
               "\",\"sdk\":" + Build.VERSION.SDK_INT +
               ",\"arch\":\"" + Build.SUPPORTED_ABIS[0] + "\"}";
    }

    @JavascriptInterface
    public void log(String message) {
        Log.d(TAG, message);
    }

    @JavascriptInterface
    public boolean isAndroid() {
        return true;
    }

    // ========== LiteRT Integration ==========
    private static LiteRTManager liteRTManager;
    private static Context appContext;

    public static void initLiteRT(String modelPath) {
        if (liteRTManager == null && appContext != null) {
            liteRTManager = new LiteRTManager(appContext);
        }
        if (liteRTManager != null) {
            new Thread(() -> {
                boolean success = BuildersKt.runBlocking(
                    Dispatchers.getIO(),
                    (scope, cont) -> liteRTManager.initialize(modelPath, cont)
                );
                // Optional: panggil native callback
            }).start();
        }
    }

    public static String sendPrompt(String prompt) {
        if (liteRTManager == null) return "";
        return BuildersKt.runBlocking(
            Dispatchers.getIO(),
            (scope, cont) -> liteRTManager.sendMessageSync(prompt, cont)
        );
    }

    public static void shutdownLiteRT() {
        if (liteRTManager != null) {
            liteRTManager.shutdown();
            liteRTManager = null;
        }
    }
}
