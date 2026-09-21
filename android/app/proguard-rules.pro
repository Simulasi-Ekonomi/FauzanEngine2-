# Keep the canonical JNI lifecycle bridge and its native entry points.
-keep class com.neoengine.core.NeoEngineCanonicalBridge { *; }

# Keep the JavaScript bridge methods exposed to the editor WebView.
-keepclassmembers class com.neoengine.core.NeoEngineBridge {
    @android.webkit.JavascriptInterface <methods>;
}
