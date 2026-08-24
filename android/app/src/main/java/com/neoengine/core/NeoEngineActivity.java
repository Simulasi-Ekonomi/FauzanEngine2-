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

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        getWindow().setFlags(
            WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN
        );

        webView = new WebView(this);
        setContentView(webView);
        // ===== TEST BUTTON FOR WORLD STREAMING =====
        android.widget.Button testWorldBtn = new android.widget.Button(this);
        testWorldBtn.setText("🌍 Generate 10km World");
        testWorldBtn.setBackgroundColor(0xFF2a5a2a);
        testWorldBtn.setTextColor(0xFFFFFFFF);
        testWorldBtn.setOnClickListener(v -> {
            NeoEngineBridge.startWorldStreaming(12345, 10.0f);
            testWorldBtn.setText("⏳ Generating...");
            testWorldBtn.setEnabled(false);
        });
        android.widget.FrameLayout.LayoutParams params = new android.widget.FrameLayout.LayoutParams(
            android.widget.FrameLayout.LayoutParams.WRAP_CONTENT,
            android.widget.FrameLayout.LayoutParams.WRAP_CONTENT);
        params.gravity = android.view.Gravity.BOTTOM | android.view.Gravity.END;
        params.setMargins(0, 0, 16, 16);
        addContentView(testWorldBtn, params);

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
        
        // Copy LiteRT model from sdcard to internal storage (hanya satu kali)
        java.io.File modelFile = new java.io.File("/sdcard/Gemma4/gemma4_2b_v09_obfus_fix_all_modalities_thinking.litertlm");
        java.io.File destFile = new java.io.File(getFilesDir(), "gemma4.litertlm");
        if (!destFile.exists() && modelFile.exists()) {
            try {
                java.io.FileInputStream fis = new java.io.FileInputStream(modelFile);
                java.io.FileOutputStream fos = new java.io.FileOutputStream(destFile);
                byte[] buf = new byte[8192];
                int len;
                while ((len = fis.read(buf)) > 0) fos.write(buf, 0, len);
                fis.close(); fos.close();
            } catch (Exception e) { e.printStackTrace(); }
        }
        if (destFile.exists()) {
            NeoEngineBridge.initLiteRT(destFile.getAbsolutePath());
        }
        
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
        super.onPause();
        if (webView != null) webView.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (webView != null) webView.onResume();
    }

    @Override
    protected void onDestroy() {
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
