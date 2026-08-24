#pragma once
#include "AI/V4/AITacticalSystem.h"
#include "AI/Gemma4Integration.h"
#include "AI/HermesIntegration.h"
#include "AI/RufloIntegration.h"
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>

namespace NeoEngine {

struct AIBridgeRequest {
    uint32_t entityID;
    std::string prompt;
    int priority = 0; // 0=low, 1=medium, 2=high
    std::string targetModel; // "gemma", "hermes", "ruflo"
};

struct AIBridgeResponse {
    uint32_t entityID;
    std::string text;
    bool success = false;
};

class AIBridge {
public:
    AIBridge() : running_(false) {}
    ~AIBridge() { Shutdown(); }

    bool Initialize() {
        // Inisialisasi semua backend AI
        gemma_.Initialize(Gemma4ModelSize::Base);
        hermes_.Initialize(HermesModelType::Medium);
        ruflo_.Initialize(ExecutionContextType::Sandbox);

        LOGI("AIBridge: Gemma=%s, Hermes=%s, Ruflo=%s",
            gemma_.IsReady() ? "OK" : "N/A",
            hermes_.IsReady() ? "OK" : "N/A",
            ruflo_.IsReady() ? "OK" : "N/A");

        // Jalankan worker thread untuk pemrosesan async
        running_ = true;
        workerThread_ = std::thread(&AIBridge::ProcessLoop, this);
        return gemma_.IsReady() || hermes_.IsReady();
    }

    void Shutdown() {
        running_ = false;
        if (workerThread_.joinable()) workerThread_.join();
    }

    // ECS memanggil ini untuk mengirim permintaan AI (non-blocking)
    void PushRequest(uint32_t entityID, const std::string& prompt, 
                     const std::string& model = "gemma", int priority = 0) {
        std::lock_guard<std::mutex> lock(queueMutex_);
        requestQueue_.push({entityID, prompt, priority, model});
    }

    // ECS memanggil ini setiap frame untuk mengambil respons yang sudah siap
    bool PollResponse(uint32_t& outEntityID, std::string& outText) {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (responseQueue_.empty()) return false;
        auto resp = responseQueue_.front();
        responseQueue_.pop();
        outEntityID = resp.entityID;
        outText = resp.text;
        return resp.success;
    }

    // Akses langsung ke backend (untuk use case khusus)
    Gemma4Integration& GetGemma() { return gemma_; }
    HermesIntegration& GetHermes() { return hermes_; }
    RufloIntegration& GetRuflo() { return ruflo_; }

private:
    void ProcessLoop() {
        while (running_) {
            AIBridgeRequest req;
            bool hasRequest = false;
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                if (!requestQueue_.empty()) {
                    req = requestQueue_.front();
                    requestQueue_.pop();
                    hasRequest = true;
                }
            }

            if (hasRequest) {
                std::string response;
                bool success = false;

                if (req.targetModel == "gemma" && gemma_.IsReady()) {
                    auto gemmaResp = gemma_.GenerateText(req.prompt, 256);
                    response = gemmaResp.generatedText;
                    success = !response.empty();
                } else if (req.targetModel == "hermes" && hermes_.IsReady()) {
                    auto hermesResp = hermes_.GenerateText(req.prompt);
                    response = hermesResp.text;
                    success = !response.empty();
                } else if (req.targetModel == "ruflo" && ruflo_.IsReady()) {
                    auto rufloResp = ruflo_.ExecuteCode(req.prompt, "text");
                    response = rufloResp.stdout;
                    success = rufloResp.success;
                }

                if (success) {
                    std::lock_guard<std::mutex> lock(queueMutex_);
                    responseQueue_.push({req.entityID, response, true});
                }
            } else {
                // Sleep untuk mengurangi CPU usage
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }

    Gemma4Integration gemma_;
    HermesIntegration hermes_;
    RufloIntegration ruflo_;

    std::atomic<bool> running_;
    std::thread workerThread_;
    std::queue<AIBridgeRequest> requestQueue_;
    std::queue<AIBridgeResponse> responseQueue_;
    std::mutex queueMutex_;
};

} // namespace NeoEngine
