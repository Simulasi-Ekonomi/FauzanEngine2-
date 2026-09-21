#include "RufloIntegration.h"
#include <curl/curl.h>
#include <json/json.h>
#include <android/log.h>
#include <algorithm>

#define LOG_TAG "RufloIntegration"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    const size_t totalSize = size * nmemb;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

RufloIntegration::RufloIntegration() : ready(false), contextType(ExecutionContextType::Sandbox), runtimeHandle(nullptr), timeoutMs(5000) {}
RufloIntegration::~RufloIntegration() { Shutdown(); }

bool RufloIntegration::Initialize(ExecutionContextType ctx) {
    contextType = ctx;
    CURL* curl = curl_easy_init();
    if (!curl) { ready = false; return false; }
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:5000/health");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
    const CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    ready = (res == CURLE_OK);
    LOGI("Ruflo server %s", ready ? "connected" : "unavailable");
    return ready;
}

void RufloIntegration::Shutdown() { ready = false; }

ExecutionResult RufloIntegration::ExecuteCode(const std::string& code, const std::string& lang) {
    if (!ValidateCode(code, lang)) return {1, "", "Ruflo request rejected", 0.0f, false};
    CURL* curl = curl_easy_init();
    if (!curl) return {1, "", "CURL init failed", 0.0f, false};

    Json::Value body;
    body["code"] = code;
    body["language"] = lang;
    Json::FastWriter writer;
    const std::string jsonBody = writer.write(body);
    std::string responseStr;
    struct curl_slist* headers = curl_slist_append(nullptr, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:5000/execute");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStr);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, std::max(1, timeoutMs / 1000));
    const CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) return {1, "", "Ruflo request failed", 0.0f, false};

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(responseStr, root) || !root.isObject() || !root.isMember("success") ||
        !root["success"].isBool()) return {1, "", "Invalid response from Ruflo", 0.0f, false};

    ExecutionResult result{};
    result.exitCode = root.get("exitCode", 1).asInt();
    result.stdout = root.get("stdout", "").asString();
    result.stderr = root.get("stderr", "").asString();
    result.executionTime = root.get("executionTime", 0.0f).asFloat();
    result.success = root["success"].asBool();
    return result;
}

ExecutionResult RufloIntegration::ExecuteWithEnvironment(const std::string& code, const std::string& lang,
                                                         const std::map<std::string, std::string>& env) {
    if (env.size() > 128U) return {1, "", "Environment limit exceeded", 0.0f, false};
    for (const auto& [key, value] : env) {
        if (key.empty() || key.size() > 256U || value.size() > 4096U) return {1, "", "Invalid environment", 0.0f, false};
    }
    return ExecuteCode(code, lang);
}

bool RufloIntegration::ValidateCode(const std::string& code, const std::string& lang) {
    const auto languages = GetSupportedLanguages();
    return ready && !code.empty() && code.size() <= 1024U * 1024U &&
           std::find(languages.begin(), languages.end(), lang) != languages.end();
}

std::vector<std::string> RufloIntegration::GetSupportedLanguages() const {
    return {"cpp", "python", "javascript", "typescript", "java", "csharp"};
}

bool RufloIntegration::IsReady() const { return ready; }
void RufloIntegration::SetTimeout(int ms) { if (ms > 0 && ms <= 120000) timeoutMs = ms; }

}
