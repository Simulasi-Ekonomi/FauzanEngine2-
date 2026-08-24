#include "HermesEngineIntegration.h"
#include <curl/curl.h>
#include <json/json.h>
#include <android/log.h>
#include <filesystem>
#include <thread>
#include <chrono>

#define LOG_TAG "HermesEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

HermesEngineIntegration::HermesEngineIntegration() = default;
HermesEngineIntegration::~HermesEngineIntegration() { Shutdown(); }

bool HermesEngineIntegration::Initialize(const std::string& modelPath, const std::string& serverAddress) {
    if (m_Ready) return true;

    m_ModelPath = modelPath;
    m_ServerAddress = serverAddress.empty() ? "http://localhost:11434" : serverAddress;

    // Cek koneksi ke Ollama
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOGE("Failed to initialize libcurl");
        return false;
    }

    std::string healthUrl = m_ServerAddress + "/api/tags";  // Ollama health check
    curl_easy_setopt(curl, CURLOPT_URL, healthUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);

    std::string responseStr;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStr);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOGI("Ollama server not reachable at %s (will retry on first use)", healthUrl.c_str());
        // Tetap set ready = true untuk lazy initialization
    } else {
        LOGI("Ollama server connected: %s", responseStr.c_str());
    }

    m_Ready = true;
    LOGI("HermesEngineIntegration initialized (server: %s)", m_ServerAddress.c_str());
    return true;
}

void HermesEngineIntegration::Shutdown() {
    m_Ready = false;
    m_ModelHandle = nullptr;
}

HermesEngineResponse HermesEngineIntegration::GenerateText(const std::string& prompt, float temperature, int maxTokens) {
    if (!m_Ready) return {"", 0.0f, 0, ""};

    Json::Value body;
    body["model"] = m_ModelPath.empty() ? "hermes3:latest" : m_ModelPath;
    body["prompt"] = prompt;
    body["stream"] = false;
    Json::Value options;
    options["temperature"] = temperature;
    options["num_predict"] = maxTokens;
    body["options"] = options;

    Json::FastWriter writer;
    std::string jsonBody = writer.write(body);

    std::string response = HttpPost(m_ServerAddress + "/api/generate", jsonBody);

    Json::Value root;
    Json::Reader reader;
    if (reader.parse(response, root)) {
        HermesEngineResponse result;
        result.text = root["response"].asString();
        result.confidence = 0.9f;
        result.tokenCount = root.get("eval_count", 0).asInt();
        result.modelInfo = "Hermes Engine (Ollama)";
        return result;
    }

    return {"Error parsing response", 0.0f, 0, ""};
}

HermesEngineResponse HermesEngineIntegration::Chat(const std::string& message, int contextSize) {
    if (!m_Ready) return {"", 0.0f, 0, ""};

    Json::Value body;
    body["model"] = m_ModelPath.empty() ? "hermes3:latest" : m_ModelPath;
    Json::Value messages(Json::arrayValue);
    Json::Value msg;
    msg["role"] = "user";
    msg["content"] = message;
    messages.append(msg);
    body["messages"] = messages;
    body["stream"] = false;
    Json::Value options;
    options["num_predict"] = contextSize;
    body["options"] = options;

    Json::FastWriter writer;
    std::string jsonBody = writer.write(body);

    std::string response = HttpPost(m_ServerAddress + "/api/chat", jsonBody);

    Json::Value root;
    Json::Reader reader;
    if (reader.parse(response, root)) {
        HermesEngineResponse result;
        result.text = root["message"]["content"].asString();
        result.confidence = 0.9f;
        result.tokenCount = root.get("eval_count", 0).asInt();
        result.modelInfo = "Hermes Engine Chat (Ollama)";
        return result;
    }

    return {"Error parsing response", 0.0f, 0, ""};
}

HermesEngineResponse HermesEngineIntegration::CodeGeneration(const std::string& description) {
    std::string prompt = "You are an expert C++ game engine programmer. Generate code for the following requirement:\n" 
                         + description + "\n```cpp\n";
    return GenerateText(prompt, 0.2f, 4096);
}

std::string HermesEngineIntegration::GetModelInfo() const {
    return "Hermes Engine 6.1GB via Ollama (" + m_ServerAddress + ")";
}

std::string HermesEngineIntegration::HttpPost(const std::string& endpoint, const std::string& jsonBody) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string responseStr;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStr);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);  // 5 menit untuk model besar

    curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return responseStr;
}

} // namespace NeoEngine
