

# Try to load JNI bridge for local Gemma 4 (if not already loaded)
if JNIUS_AVAILABLE:
    try:
        NeoEngineBridge = autoclass('com.neoengine.core.NeoEngineBridge')
        # Ensure LiteRT is initialized with Gemma4 model path if needed
        # We assume the Java side has been initialized via Android app
        # For simplicity, we call a static initLiteRT with default model path if not already done.
        # This requires Android context; we'll skip initialization here and assume it's done.
        # In a real scenario, you'd pass Context from the Android activity.
        # For now, we just rely on the existing LiteRTManager state.
        print('[ARIES] JNI bridge to NeoEngineBridge loaded')
    except Exception as e:
        print(f'[ARIES] Failed to load JNI bridge: {e}')
        JNIUS_AVAILABLE = False

# Try loading the actual sovereign brain components