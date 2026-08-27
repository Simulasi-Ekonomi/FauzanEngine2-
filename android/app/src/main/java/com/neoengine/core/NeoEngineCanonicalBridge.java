package com.neoengine.core;

import android.util.Log;

/**
 * Narrow lifecycle-only bridge for the audited native Android subset.
 * A failed native load is reported as false rather than being retried through legacy native APIs.
 */
public final class NeoEngineCanonicalBridge {
    private static final String TAG = "NeoEngineCanonicalBridge";
    private static boolean nativeAvailable;

    static {
        try {
            System.loadLibrary("neo_core");
            nativeAvailable = true;
        } catch (UnsatisfiedLinkError error) {
            nativeAvailable = false;
            Log.w(TAG, "Canonical native library is unavailable", error);
        }
    }

    private NeoEngineCanonicalBridge() {}

    public static boolean initialize() { return invoke(0, 0.0f); }
    public static boolean resume() { return invoke(1, 0.0f); }
    public static boolean pause() { return invoke(2, 0.0f); }
    public static boolean tick(float deltaSeconds) { return invoke(3, deltaSeconds); }
    public static boolean shutdown() { return invoke(4, 0.0f); }

    public static boolean isNativeAvailable() { return nativeAvailable; }

    public static String coreProfile() {
        if (!nativeAvailable) return "NATIVE_UNAVAILABLE";
        try {
            return nativeCoreProfile();
        } catch (UnsatisfiedLinkError error) {
            nativeAvailable = false;
            Log.w(TAG, "Canonical native profile call failed", error);
            return "NATIVE_UNAVAILABLE";
        }
    }

    private static boolean invoke(int eventCode, float deltaSeconds) {
        if (!nativeAvailable) return false;
        try {
            return nativeLifecycleEvent(eventCode, deltaSeconds);
        } catch (UnsatisfiedLinkError error) {
            nativeAvailable = false;
            Log.w(TAG, "Canonical native lifecycle call failed", error);
            return false;
        }
    }

    private static native boolean nativeLifecycleEvent(int eventCode, float deltaSeconds);
    private static native String nativeCoreProfile();
}
