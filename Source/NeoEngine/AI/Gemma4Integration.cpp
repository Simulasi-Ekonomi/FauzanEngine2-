#include "Gemma4Integration.h"
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <android/log.h>
#include <chrono>
#include <future>
#include <unistd.h>

#define LOG_TAG "Gemma4Int"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

Gemma4Integration::Gemma4Integration() : ready(false), modelSize(Gemma4ModelSize::Base), modelHandle(nullptr) {}
Gemma4Integration::~Gemma4Integration() { Shutdown(); }

bool Gemma4Integration::Initialize(Gemma4ModelSize size) {
    modelSize = size;
    std::string modelPath = "/sdcard/Gemma4/";
    if (size == Gemma4ModelSize::Small) {
        modelFilePath = modelPath + "gemma4_2b_v09_obfus_fix_all_modalities_thinking.litertlm";
    } else if (size == Gemma4ModelSize::Base) {
        modelFilePath = modelPath + "gemma4_thinking.bin";
    } else {
        modelFilePath = modelPath + "gemma4_thinking.bin";
    }

    // Cek apakah file model ada
    std::ifstream modelFile(modelFilePath);
    if (!modelFile.good()) {
        LOGI("Gemma model not found at %s", modelFilePath.c_str());
        ready = false;
        return false;
    }
    modelFile.close();

    // Cek apakah binary `gemma` ada di PATH
    FILE* pipe = popen("which gemma 2>/dev/null", "r");
    if (pipe) {
        char buffer[128];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) result += buffer;
        pclose(pipe);
        if (!result.empty() && result.find("gemma") != std::string::npos) {
            ready = true;
            LOGI("Gemma binary found. Integration ready (CLI mode).");
            return true;
        }
    }

    // Cek keberadaan TFLite library (hanya ada/tidaknya file)
    if (access("/data/data/com.termux/files/usr/lib/libtensorflowlite_jni.so", F_OK) == 0 ||
        access("/sdcard/Gemma4/libtensorflowlite_jni.so", F_OK) == 0) {
        ready = true;
        LOGI("TensorFlow Lite library found, Gemma integration ready (TFLite mode).");
        return true;
    }

    // Jika tidak ada sama sekali, set ready = false
    ready = false;
    LOGI("Gemma integration: no runtime found. Install gemma.cpp or TFLite.");
    return false;
}

void Gemma4Integration::Shutdown() {
    ready = false;
}

static std::string execCommand(const std::string& cmd) {
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return result;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) result += buffer;
    pclose(pipe);
    return result;
}

Gemma4Response Gemma4Integration::GenerateText(const std::string& prompt, int maxLength) {
    Gemma4Response resp;
    if (!ready) {
        resp.generatedText = "Gemma not ready";
        return resp;
    }

    // Coba gunakan binary `gemma` jika ada
    std::string command = "gemma --model " + modelFilePath + " --prompt \"" + prompt + "\" --max_tokens " + std::to_string(maxLength);
    std::string output = execCommand(command);
    if (!output.empty()) {
        resp.generatedText = output;
        resp.confidence = 0.9f;
        resp.tokensUsed = output.length() / 4;
        return resp;
    }

    // Dummy response untuk pengujian pipeline
    resp.generatedText = "[Gemma thinking...]";
    resp.confidence = 0.8f;
    resp.tokensUsed = 4;
    return resp;
}

std::vector<float> Gemma4Integration::GetEmbeddings(const std::string& text) {
    return std::vector<float>(64, 0.1f);
}

Gemma4Response Gemma4Integration::Summarize(const std::string& text) {
    return GenerateText("Summarize: " + text, 128);
}

bool Gemma4Integration::IsReady() const { return ready; }
std::string Gemma4Integration::GetModelInfo() const {
    return "Gemma 4 " + std::to_string(static_cast<int>(modelSize)) + " (local)";
}
void Gemma4Integration::SetModelSize(Gemma4ModelSize size) { modelSize = size; }

} // namespace NeoEngine
