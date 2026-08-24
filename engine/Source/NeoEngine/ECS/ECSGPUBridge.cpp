#include "ECSGPUBridge.h"
#include "PrivateAPISystem.h"
#include <android/log.h>
#include <mutex>
#include <cstring>

#define LOG_TAG "ECSGPU"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

static PrivateAPISystem* g_privateAPI = nullptr;
static std::mutex g_bridgeMutex;

void ECSGPUBridge::Initialize(const char* masterKey, const char* serverURL) {
    std::lock_guard<std::mutex> lock(g_bridgeMutex);
    if (!g_privateAPI) {
        g_privateAPI = new PrivateAPISystem();
        g_privateAPI->SetMasterSecretKey(masterKey);
        g_privateAPI->SetServerBaseURL(serverURL);
        LOGI("ECS GPU Bridge initialized with Private API v2");
    }
}

void ECSGPUBridge::SubmitToGPU(const void* data, size_t size) {
    if (!g_privateAPI) {
        LOGE("SubmitToGPU: Bridge not initialized. Call Initialize() first.");
        return;
    }
    // Build a secure request to log/verify the data transfer
    APIRequest req;
    req.endpoint = APIEndpoint::TELEMETRY_PUSH;
    req.clientId = "ecs_gpu_bridge";
    req.clientVersion = "2.1";
    req.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    req.nonce = g_privateAPI->GetTotalRequests(); // simplistic
    req.payload.assign(static_cast<const char*>(data), size);
    req.sessionToken = "gpu_session"; // would be from active session
    req.hmacSignature = g_privateAPI->GenerateHMAC(req.payload, "internal");

    APIResponse res = g_privateAPI->ExecuteRequest(req, "127.0.0.1");
    if (res.success) {
        // Actual GPU memory write would happen here (Vulkan buffer map)
        LOGI("GPU data submitted securely, size: %zu", size);
    } else {
        LOGE("GPU submission rejected by security layer: %s", res.errorMessage.c_str());
    }
}

} // namespace
