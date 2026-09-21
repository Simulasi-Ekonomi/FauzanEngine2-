package com.neoengine.core;

import android.app.Activity;
import android.os.Bundle;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.webkit.WebSettings;
import android.webkit.JavascriptInterface;
import android.webkit.WebChromeClient;
import android.view.WindowManager;
import android.os.Build;
import android.view.Choreographer;

public class NeoEngineActivity extends Activity {
    private WebView webView;
    private NeoEngineBridge bridge;
    private boolean nativeInitialized;
    private boolean nativeActive;
    private final Choreographer.FrameCallback frameCallback = this::onEngineFrame;
    private long lastFrameNanos;
    private boolean frameLoopScheduled;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        getWindow().setFlags(
            WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN
        );

        webView = new WebView(this);
        setContentView(webView);
        WebSettings settings = webView.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        settings.setAllowFileAccess(true);
        settings.setAllowContentAccess(true);
        settings.setMediaPlaybackRequiresUserGesture(false);
        settings.setDatabaseEnabled(true);
        
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            settings.setSafeBrowsingEnabled(false);
        }

        webView.setWebChromeClient(new WebChromeClient());

        bridge = new NeoEngineBridge();
        bridge.init(this);
        nativeInitialized = NeoEngineCanonicalBridge.initialize();
        nativeActive = false;
        lastFrameNanos = 0L;
        frameLoopScheduled = false;
        
        webView.addJavascriptInterface(bridge, "NeoEngineBridge");
        webView.addJavascriptInterface(this, "AndroidActivity");

        webView.setWebViewClient(new WebViewClient() {
            @Override
            public void onPageFinished(WebView view, String url) {
                view.evaluateJavascript(
                    "if(window.onNeoEngineReady) window.onNeoEngineReady()", null
                );
            }
        });

        webView.loadUrl("file:///android_asset/editor/index.html");
    }

    @JavascriptInterface
    public String getDeviceInfo() {
        return "{\"model\":\"" + Build.MODEL + "\",\"sdk\":" + Build.VERSION.SDK_INT + "}";
    }

    @Override
    protected void onPause() {
        if (nativeActive && NeoEngineCanonicalBridge.pause()) nativeActive = false;
        super.onPause();
        if (webView != null) webView.onPause();
        stopNativeFrameLoop();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (webView != null) webView.onResume();
        if (nativeInitialized && !nativeActive) nativeActive = NeoEngineCanonicalBridge.resume();
        if (nativeActive) startNativeFrameLoop();
    }

    private void startNativeFrameLoop() {
        if (frameLoopScheduled || !nativeActive) return;
        lastFrameNanos = System.nanoTime();
        frameLoopScheduled = true;
        Choreographer.getInstance().postFrameCallback(frameCallback);
    }

    private void stopNativeFrameLoop() {
        if (!frameLoopScheduled) return;
        Choreographer.getInstance().removeFrameCallback(frameCallback);
        frameLoopScheduled = false;
        lastFrameNanos = 0L;
    }

    private void onEngineFrame(long frameTimeNanos) {
        frameLoopScheduled = false;
        if (!nativeActive || isFinishing() || (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1 && isDestroyed())) return;
        float deltaSeconds = lastFrameNanos == 0L ? (1.0f / 60.0f) : (frameTimeNanos - lastFrameNanos) / 1_000_000_000.0f;
        lastFrameNanos = frameTimeNanos;
        if (!Float.isFinite(deltaSeconds) || deltaSeconds < 0.0f) deltaSeconds = 1.0f / 60.0f;
        deltaSeconds = Math.min(deltaSeconds, 0.25f);
        if (!NeoEngineCanonicalBridge.tick(deltaSeconds)) {
            nativeActive = false;
            stopNativeFrameLoop();
            return;
        }
        startNativeFrameLoop();
    }

    @Override
    protected void onDestroy() {
        stopNativeFrameLoop();
        if (nativeActive) NeoEngineCanonicalBridge.pause();
        if (nativeInitialized) NeoEngineCanonicalBridge.shutdown();
        nativeActive = false;
        nativeInitialized = false;
        if (bridge != null) bridge.shutdown();
        NeoEngineBridge.shutdownLiteRT();
        if (webView != null) {
            webView.destroy();
            webView = null;
        }
        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
        if (webView != null && webView.canGoBack()) {
            webView.goBack();
        } else {
            super.onBackPressed();
        }
    }
}