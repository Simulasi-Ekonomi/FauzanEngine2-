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

public class NeoEngineActivity extends Activity {
    private WebView webView;
    private NeoEngineBridge bridge;
    private boolean nativeInitialized;
    private boolean nativeActive;

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
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (webView != null) webView.onResume();
        if (nativeInitialized && !nativeActive) nativeActive = NeoEngineCanonicalBridge.resume();
    }

    @Override
    protected void onDestroy() {
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
