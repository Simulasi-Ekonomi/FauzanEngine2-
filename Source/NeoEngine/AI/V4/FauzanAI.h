#pragma once
#include "AI/Gemma4Integration.h"
#include "AI/HermesIntegration.h"
#include "AI/RufloIntegration.h"
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <algorithm>
#include <ctime>
#include <cmath>
#include <cctype>
#include <stdexcept>
#include <condition_variable>

namespace NeoEngine {

enum class AILevel : uint8_t { Tactical = 0, Strategic = 1, Global = 2 };

struct AITask {
    uint32_t entityID; std::string prompt;
    AILevel level; std::string targetModel;
    float nextUpdate = 0.0f; int priority = 0;
    float waitingTime = 0.0f;
    std::vector<float> embedding;
    int retryCount = 0;
};

// SPSC ring buffer (lock-free, cache-aligned)
template<typename T, size_t CAP>
class SPSCQueue {
    alignas(64) struct AtomicIndex { std::atomic<size_t> v{0}; } writePos_, readPos_;
    T buffer_[CAP];
public:
    bool push(const T& item) {
        size_t w = writePos_.v.load(std::memory_order_relaxed);
        size_t r = readPos_.v.load(std::memory_order_acquire);
        if ((w - r) >= CAP) return false;
        buffer_[w % CAP] = item;
        writePos_.v.store(w + 1, std::memory_order_release);
        return true;
    }
    bool pop(T& item) {
        size_t r = readPos_.v.load(std::memory_order_relaxed);
        size_t w = writePos_.v.load(std::memory_order_acquire);
        if (w <= r) return false;
        item = buffer_[r % CAP];
        readPos_.v.store(r + 1, std::memory_order_release);
        return true;
    }
};

struct AICommand {
    uint8_t commandID = 0;
    float confidence = 0.0f;
};

class FauzanAI {
public:
    FauzanAI() : running_(false) { InitGLMExperts(); }
    ~FauzanAI() { Shutdown(); }

    bool Initialize() {
        gemma_.Initialize(Gemma4ModelSize::Base);
        hermes_.Initialize(HermesModelType::Medium);
        ruflo_.Initialize(ExecutionContextType::Sandbox);
        running_=true;
        schedulerThread_ = std::thread(&FauzanAI::SchedulerLoop, this);
        workerTactical_ = std::thread(&FauzanAI::WorkerLoop, this, AILevel::Tactical);
        workerStrategic_ = std::thread(&FauzanAI::WorkerLoop, this, AILevel::Strategic);
        return gemma_.IsReady() || hermes_.IsReady();
    }

    void Shutdown() {
        running_.store(false,std::memory_order_release);
        cvScheduler_.notify_all();
        if(schedulerThread_.joinable()) schedulerThread_.join();
        if(workerTactical_.joinable()) workerTactical_.join();
        if(workerStrategic_.joinable()) workerStrategic_.join();
    }

    bool SubmitTask(uint32_t entityID, const std::string& prompt, AILevel level,
                    const std::string& model="gemma", int priority=0) {
        std::lock_guard<std::mutex> lock(schedulerMutex_);
        if (pendingTasks_.size() >= 5000) return false;
        AITask t; t.entityID=entityID; t.prompt=prompt; t.level=level;
        t.targetModel=model; t.priority=priority; t.nextUpdate=0.0f; t.waitingTime=0.0f;
        pendingTasks_.push_back(t);
        cvScheduler_.notify_one();
        return true;
    }

    bool PollAICommand(uint32_t& outEntityID, AICommand& outCmd) {
        std::pair<uint32_t, AICommand> item;
        if (completedCommands_.pop(item)) {
            outEntityID = item.first;
            outCmd = item.second;
            return true;
        }
        return false;
    }

    std::vector<int> RouteExperts(const std::vector<float>& input, int topK=8) {
        if (++frameCount_ % 100 == 0) {
            for (auto& kv : expertUsage_) kv.second = (int)(kv.second * 0.5f);
        }
        if (input.empty()) return {};

        float mean = 0.0f, var = 0.0f;
        for(auto v : input) mean += v;
        mean /= input.size();
        for(auto v : input) var += (v - mean) * (v - mean);
        var = sqrtf(var / input.size() + 1e-5f);
        std::vector<float> normInput = input;
        for(auto& v : normInput) v = (v - mean) / var;

        std::vector<std::pair<float,int>> scores;
        float maxScore = -1e9f;
        for (auto& expert : glmState_.experts) {
            float score = 0.0f;
            for (size_t i=0; i<normInput.size() && i<expert.params.size(); ++i)
                score += normInput[i] * expert.params[i];
            score -= logf(1.0f + expertUsage_[expert.id]) * 0.05f;
            // Noise kecil untuk mencegah cold start bias
            score += (rand() % 2000 - 1000) / 100000.0f;
            scores.push_back({score, expert.id});
            if (score > maxScore) maxScore = score;
        }
        float sumExp = 0.0f;
        for (auto& s : scores) {
            s.first = expf((s.first - maxScore) / rdtConfig_.glmTemperature);
            sumExp += s.first;
        }
        if (sumExp > 0.0f) for (auto& s : scores) s.first /= sumExp;
        std::partial_sort(scores.begin(), scores.begin()+topK, scores.end(),
            [](auto& a, auto& b){ return a.first > b.first; });
        std::vector<int> selected;
        for (int i=0; i<topK; ++i) {
            selected.push_back(scores[i].second);
            expertUsage_[scores[i].second]++;
        }
        return selected;
    }

    Gemma4Integration& GetGemma() { return gemma_; }
    HermesIntegration& GetHermes() { return hermes_; }
    RufloIntegration& GetRuflo() { return ruflo_; }

private:
    void InitGLMExperts() {
        for (int i=0; i<256; ++i) {
            GLMExpert e; e.id=i; e.params.resize(64, 0.0f);
            for(auto& v:e.params) v=(rand()%1000)/1000.0f-0.5f;
            glmState_.experts.push_back(e); expertUsage_[i]=0;
        }
    }

    void SchedulerLoop() {
        while(running_.load(std::memory_order_acquire)) {
            std::vector<AITask> batch;
            {
                std::unique_lock<std::mutex> lock(schedulerMutex_);
                cvScheduler_.wait_for(lock, std::chrono::milliseconds(50), [this]{ return !running_.load(std::memory_order_acquire) || !pendingTasks_.empty(); });
                if (!running_.load(std::memory_order_acquire)) break;
                batch.swap(pendingTasks_);
            }

            for(auto& t:batch) {
                // Pisahkan berdasarkan priority: priority>0 masuk antrean tinggi
                bool ok;
                if (t.priority > 0) {
                    ok = (t.level == AILevel::Tactical) ?
                        tacticalHighQueue_.push(t) : strategicHighQueue_.push(t);
                } else {
                    ok = (t.level == AILevel::Tactical) ?
                        tacticalQueue_.push(t) : strategicQueue_.push(t);
                }
                if (!ok) {
                    if (t.level == AILevel::Tactical) tacticalOverflow_.push(t);
                    else strategicOverflow_.push(t);
                }
            }
        }
    }

    uint8_t parseCommand(const std::string& response) {
        if (response.empty()) return 0;
        try {
            std::string trimmed = response;
            trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
            trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);
            // Cari digit pertama saja, hindari string yang tidak ada digit
            for (char c : trimmed) {
                if (isdigit(c)) {
                    int val = c - '0';
                    if (val >= 0 && val <= 4) return (uint8_t)val;
                }
            }
            // Fallback: jika semua karakter digit murni (seperti "3")
            if (std::all_of(trimmed.begin(), trimmed.end(), ::isdigit)) {
                int val = std::stoi(trimmed);
                if (val >= 0 && val <= 4) return (uint8_t)val;
            }
        } catch(...) { }
        return 0;
    }

    void WorkerLoop(AILevel assignedLevel) {
        int idleCount = 0;
        while(running_.load(std::memory_order_acquire)) {
            AITask task; bool hasWork=false;
            
            // Prioritaskan antrean tinggi, lalu normal, lalu overflow
            auto& highQueue = (assignedLevel == AILevel::Tactical) ? tacticalHighQueue_ : strategicHighQueue_;
            auto& normQueue = (assignedLevel == AILevel::Tactical) ? tacticalQueue_ : strategicQueue_;
            auto& overflow = (assignedLevel == AILevel::Tactical) ? tacticalOverflow_ : strategicOverflow_;

            if (highQueue.pop(task)) hasWork = true;
            else if (normQueue.pop(task)) hasWork = true;
            else if (overflow.pop(task)) hasWork = true;

            if(hasWork) {
                idleCount = 0;
                std::string model=task.targetModel;
                if(model=="auto") {
                    if(gemma_.IsReady()) model="gemma";
                    else if(hermes_.IsReady()) model="hermes";
                }
                std::string response;
                bool inferenceSuccess = false;
                try {
                    if(model=="gemma"&&gemma_.IsReady()) {
                        response = gemma_.GenerateText(task.prompt, 256).generatedText;
                        inferenceSuccess = true;
                    } else if(model=="hermes"&&hermes_.IsReady()) {
                        response = hermes_.GenerateText(task.prompt).text;
                        inferenceSuccess = true;
                    } else if(model=="ruflo"&&ruflo_.IsReady()) {
                        response = ruflo_.ExecuteCode(task.prompt,"text").stdout;
                        inferenceSuccess = true;
                    }
                } catch(const std::exception& e) {
                    __android_log_print(ANDROID_LOG_ERROR, "FauzanAI", "Inference failure: %s", e.what());
                } catch(...) { }

                if (!inferenceSuccess) {
                    task.retryCount++;
                    if (task.retryCount < 3) {
                        overflow.push(task);
                    }
                    continue;
                }

                if(!response.empty()) {
                    AICommand cmd;
                    cmd.commandID = parseCommand(response);
                    // Confidence dinamis: lebih tinggi jika command valid (bukan 0)
                    cmd.confidence = (cmd.commandID != 0) ? 0.85f : 0.3f;
                    completedCommands_.push({task.entityID, cmd});
                }
            } else {
                idleCount++;
                int sleepUs = std::min(200 * idleCount, 5000);
                if (assignedLevel != AILevel::Tactical)
                    sleepUs = std::min(50000 * idleCount, 200000);
                std::this_thread::sleep_for(std::chrono::microseconds(sleepUs));
            }
        }
    }

    Gemma4Integration gemma_; HermesIntegration hermes_; RufloIntegration ruflo_;
    struct GLMExpert { int id; std::vector<float> params; };
    struct GLMAgentState { std::vector<GLMExpert> experts; };
    GLMAgentState glmState_;
    std::unordered_map<int, int> expertUsage_;
    struct RDTConfig { float glmTemperature=0.7f; };
    RDTConfig rdtConfig_;
    int frameCount_ = 0;

    std::atomic<bool> running_;
    std::thread schedulerThread_, workerTactical_, workerStrategic_;
    std::vector<AITask> pendingTasks_;
    SPSCQueue<AITask, 1024> tacticalQueue_, strategicQueue_;
    SPSCQueue<AITask, 512> tacticalHighQueue_, strategicHighQueue_;
    SPSCQueue<AITask, 256> tacticalOverflow_, strategicOverflow_;
    SPSCQueue<std::pair<uint32_t, AICommand>, 1024> completedCommands_;
    std::mutex schedulerMutex_;
    std::condition_variable cvScheduler_;
};

} // namespace
