#include "Gemma4Integration.h"
#include <android/log.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

#define LOG_TAG "Gemma4Int"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

namespace {

std::string ShellQuote(const std::string& value) {
    std::string quoted;
    quoted.reserve(value.size() + 2U);
    quoted.push_back('\'');
    for (const char c : value) {
        if (c == '\'') quoted += "'\\''";
        else quoted.push_back(c);
    }
    quoted.push_back('\'');
    return quoted;
}

bool CommandExists(const char* command) {
    if (command == nullptr || *command == '\0') return false;
    std::string probe = "command -v ";
    probe += ShellQuote(command);
    probe += " >/dev/null 2>&1";
    const int status = std::system(probe.c_str());
    return status != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

std::string ExecuteGemma(const std::string& modelPath, const std::string& prompt, int maxTokens, int& exitCode) {
    exitCode = -1;
    const int boundedTokens = std::clamp(maxTokens, 1, 32768);
    const std::string command =
        "gemma --model " + ShellQuote(modelPath) +
        " --prompt " + ShellQuote(prompt) +
        " --max_tokens " + std::to_string(boundedTokens) + " 2>/dev/null";

    FILE* pipe = popen(command.c_str(), "r");
    if (pipe == nullptr) return {};

    std::string output;
    output.reserve(4096U);
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output.append(buffer);
        if (output.size() > 16U * 1024U * 1024U) {
            pclose(pipe);
            return {};
        }
    }

    const int status = pclose(pipe);
    if (status != -1 && WIFEXITED(status)) exitCode = WEXITSTATUS(status);
    return output;
}

} // namespace

Gemma4Integration::Gemma4Integration()
    : ready(false), modelSize(Gemma4ModelSize::Base), modelHandle(nullptr) {}

Gemma4Integration::~Gemma4Integration() {
    Shutdown();
}

std::string Gemma4Integration::GetDefaultModelPath() const {
    const char* overridePath = std::getenv("NEO_GEMMA_MODEL");
    if (overridePath != nullptr && *overridePath != '\0') return overridePath;

    const std::string root = "/sdcard/Gemma4/";
    switch (modelSize) {
        case Gemma4ModelSize::Small:
            return root + "gemma4_2b_v09_obfus_fix_all_modalities_thinking.litertlm";
        case Gemma4ModelSize::Base:
            return root + "gemma4_thinking.bin";
        case Gemma4ModelSize::Large:
            return root + "gemma4_thinking.bin";
    }
    return {};
}

bool Gemma4Integration::Initialize(Gemma4ModelSize size) {
    Shutdown();
    modelSize = size;
    modelFilePath = GetDefaultModelPath();

    if (modelFilePath.empty()) {
        LOGI("Gemma initialization rejected: model path is empty");
        return false;
    }

    std::ifstream modelFile(modelFilePath, std::ios::binary);
    if (!modelFile.good()) {
        LOGI("Gemma model not found at %s", modelFilePath.c_str());
        return false;
    }
    modelFile.seekg(0, std::ios::end);
    const std::streamoff modelSizeBytes = modelFile.tellg();
    if (modelSizeBytes <= 0) {
        LOGI("Gemma model is empty at %s", modelFilePath.c_str());
        return false;
    }

    if (!CommandExists("gemma")) {
        LOGI("Gemma runtime executable was not found in PATH");
        return false;
    }

    ready = true;
    LOGI("Gemma CLI integration ready: model=%s bytes=%lld",
         modelFilePath.c_str(), static_cast<long long>(modelSizeBytes));
    return true;
}

void Gemma4Integration::Shutdown() {
    ready = false;
    modelHandle = nullptr;
}

Gemma4Response Gemma4Integration::GenerateText(const std::string& prompt, int maxLength) {
    Gemma4Response response{};
    if (!ready || prompt.empty() || maxLength <= 0) return response;

    int exitCode = -1;
    const std::string output = ExecuteGemma(modelFilePath, prompt, maxLength, exitCode);
    if (exitCode != 0 || output.empty()) {
        LOGI("Gemma generation failed: exit=%d", exitCode);
        return response;
    }

    response.generatedText = output;
    response.tokensUsed = static_cast<int>((output.size() + 3U) / 4U);
    response.confidence = 1.0f;
    return response;
}

std::vector<float> Gemma4Integration::GetEmbeddings(const std::string& text) {
    // Never fabricate model embeddings. This integration currently exposes the
    // generation CLI only; callers must use a real embedding backend.
    (void)text;
    return {};
}

Gemma4Response Gemma4Integration::Summarize(const std::string& text) {
    if (text.empty()) return {};
    return GenerateText("Summarize the following text faithfully and concisely:\n\n" + text, 128);
}

bool Gemma4Integration::IsReady() const {
    return ready;
}

std::string Gemma4Integration::GetModelInfo() const {
    if (modelFilePath.empty()) return "Gemma 4 unavailable";
    return "Gemma 4 " + std::to_string(static_cast<int>(modelSize)) + " (local CLI)";
}

void Gemma4Integration::SetModelSize(Gemma4ModelSize size) {
    if (modelSize == size) return;
    const bool wasReady = ready;
    if (wasReady) Shutdown();
    modelSize = size;
    modelFilePath = GetDefaultModelPath();
}

} // namespace NeoEngine
