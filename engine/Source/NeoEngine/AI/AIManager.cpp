#include "AIManager.h"
#include <android/log.h>

#define LOG_TAG "AIManager"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

AIManager& AIManager::GetInstance() {
    static AIManager instance;
    return instance;
}

AIManager::AIManager()
    : hermes(std::make_unique<HermesIntegration>()),
      gemma4(std::make_unique<Gemma4Integration>()),
      ruflo(std::make_unique<RufloIntegration>()),
      opencode(std::make_unique<OpenCodeIntegration>()),
      initialized(false) {}

void AIManager::Initialize() {
    if (!initialized) {
        hermes->Connect();
        gemma4->Connect();
        ruflo->Connect();
        opencode->Connect();
        initialized = true;
        LOGI("AI Manager initialized with 4 backends");
    }
}

void AIManager::Shutdown() {
    if (initialized) {
        hermes->Disconnect();
        gemma4->Disconnect();
        ruflo->Disconnect();
        opencode->Disconnect();
        initialized = false;
        LOGI("AI Manager shut down");
    }
}

} // namespace NeoEngine
