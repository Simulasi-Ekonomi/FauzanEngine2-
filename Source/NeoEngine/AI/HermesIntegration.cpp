#include "HermesIntegration.h"
#include <curl/curl.h>
#include <json/json.h>
#include <android/log.h>

#define LOG_TAG "HermesIntegration"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

HermesIntegration::HermesIntegration()
    : ready(false), currentModel(HermesModelType::Medium),
      modelHandle(nullptr), temperature(0.7f), maxTokens(2048) {
    hermesEndpoint = "http://localhost:8765/v1/chat/completions";
}

HermesIntegration::~HermesIntegration() { Shutdown(); }

bool HermesIntegration::Initialize(HermesModelType modelType) {
    currentModel = modelType;
    CURL* curl = curl_easy_init();
    if (!curl) { ready = false; return false; }
    curl_easy_setopt(curl, CURLOPT_URL, (hermesEndpoint + "/../health").c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    ready = (res == CURLE_OK);
    LOGI("Hermes integration %s", ready ? "connected" : "unavailable");
    return ready;
}

void HermesIntegration::Shutdown() { ready = false; }

HermesResponse HermesIntegration::GenerateText(const std::string& prompt, float temp) {
    if (!ready) return {"", 0.0f, 0, ""};
    CURL* curl = curl_easy_init();
    if (!curl) return {"", 0.0f, 0, ""};
    Json::Value body;
    body["prompt"] = prompt;
    body["temperature"] = temp;
    body["max_tokens"] = maxTokens;
    Json::FastWriter writer;
    std::string jsonBody = writer.write(body);
    std::string responseStr;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, hermesEndpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStr);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) return {"", 0.0f, 0, ""};
    Json::Value root;
    Json::Reader reader;
    if (reader.parse(responseStr, root) && root.isMember("choices") && root["choices"].size() > 0) {
        std::string text = root["choices"][0]["message"]["content"].asString();
        return {text, 0.9f, (int)text.length() / 4, "hermes-http"};
    }
    return {responseStr, 0.8f, 128, "hermes-http"};
}

HermesResponse HermesIntegration::Chat(const std::string& msg, int ctx) {
    return GenerateText(msg, temperature);
}

HermesResponse HermesIntegration::CodeGeneration(const std::string& desc) {
    return GenerateText("Generate C++ game engine code for: " + desc, 0.2f);
}

void HermesIntegration::SetTemperature(float temp) { temperature = temp; }
void HermesIntegration::SetMaxTokens(int max) { maxTokens = max; }
bool HermesIntegration::IsReady() const { return ready; }

std::string HermesIntegration::GetModelInfo() const {
    return "Hermes Agent (HTTP)";
}

} // namespace NeoEngine
