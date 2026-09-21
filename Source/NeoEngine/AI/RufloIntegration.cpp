#include "RufloIntegration.h"
#include <curl/curl.h>
#include <json/json.h>
#include <android/log.h>
#include <algorithm>
#include <cmath>
#include <limits>

#define LOG_TAG "RufloIntegration"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    if (output == nullptr || contents == nullptr || (nmemb != 0U && size > std::numeric_limits<size_t>::max() / nmemb)) return 0U;
    const size_t totalSize = size * nmemb;
    if (totalSize > 16U * 1024U * 1024U || output->size() > 16U * 1024U * 1024U - totalSize) return 0U;
    output->append(static_cast<const char*>(contents), totalSize);
    return totalSize;
}

RufloIntegration::RufloIntegration() : ready(false), contextType(ExecutionContextType::Sandbox), runtimeHandle(nullptr), timeoutMs(5000) {}
RufloIntegration::~RufloIntegration() { Shutdown(); }

bool RufloIntegration::Initialize(ExecutionContextType ctx) {
    if (timeoutMs <= 0 || timeoutMs > 120000) return false;
    contextType = ctx;
    CURL* curl = curl_easy_init();
    if (!curl) { ready = false; return false; }
    const CURLcode urlResult = curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:5000/health");
    const CURLcode timeoutResult = curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
    if (urlResult != CURLE_OK || timeoutResult != CURLE_OK) {
        curl_easy_cleanup(curl);
        ready = false;
        return false;
    }
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
    if (headers == nullptr) {
        curl_easy_cleanup(curl);
        return {1, "", "CURL header allocation failed", 0.0f, false};
    }

    const CURLcode urlResult = curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:5000/execute");
    const CURLcode postResult = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
    const CURLcode writeResult = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    const CURLcode dataResult = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStr);
    const CURLcode headerResult = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    const long timeoutSeconds = std::max(1, timeoutMs / 1000);
    const CURLcode timeoutResult = curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSeconds);
    if (urlResult != CURLE_OK || postResult != CURLE_OK || writeResult != CURLE_OK ||
        dataResult != CURLE_OK || headerResult != CURLE_OK || timeoutResult != CURLE_OK) {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return {1, "", "Ruflo CURL configuration failed", 0.0f, false};
    }

    const CURLcode res = curl_easy_perform(curl);
    long httpStatus = 0;
    const CURLcode infoResult = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK || infoResult != CURLE_OK || httpStatus < 200L || httpStatus >= 300L ||
        responseStr.empty() || responseStr.size() > 16U * 1024U * 1024U) {
        return {1, "", "Ruflo request failed", 0.0f, false};
    }

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(responseStr, root) || !root.isObject() || !root.isMember("success") ||
        !root["success"].isBool()) return {1, "", "Invalid response from Ruflo", 0.0f, false};

    ExecutionResult result{};
    if (!root["exitCode"].isInt() || !root["stdout"].isString() || !root["stderr"].isString() || !root["executionTime"].isNumeric()) return {1, "", "Malformed Ruflo result", 0.0f, false};
    result.exitCode = root["exitCode"].asInt();
    result.stdout = root["stdout"].asString();
    result.stderr = root["stderr"].asString();
    result.executionTime = root["executionTime"].asFloat();
    if (!std::isfinite(result.executionTime) || result.executionTime < 0.0f || result.executionTime > 86400.0f) return {1, "", "Invalid execution time", 0.0f, false};
    if (result.stdout.size() > 16U * 1024U * 1024U || result.stderr.size() > 16U * 1024U * 1024U ||
        result.stdout.find('\0') != std::string::npos || result.stderr.find('\0') != std::string::npos) return {1, "", "Ruflo output limit exceeded", 0.0f, false};
    result.success = root["success"].asBool();
    return result;
}

ExecutionResult RufloIntegration::ExecuteWithEnvironment(const std::string& code, const std::string& lang,
                                                         const std::map<std::string, std::string>& env) {
    if (code.find('\0') != std::string::npos || lang.find('\0') != std::string::npos) return {1, "", "Invalid code input", 0.0f, false};
    if (env.size() > 128U) return {1, "", "Environment limit exceeded", 0.0f, false};
    for (const auto& [key, value] : env) {
        if (key.empty() || key.size() > 256U || value.size() > 4096U ||
            key.find('\0') != std::string::npos || value.find('\0') != std::string::npos) return {1, "", "Invalid environment", 0.0f};
    }
    return ExecuteCode(code, lang);
}

bool RufloIntegration::ValidateCode(const std::string& code, const std::string& lang) {
    const auto languages = GetSupportedLanguages();
    return ready && !code.empty() && code.size() <= 1024U * 1024U && code.find('\0') == std::string::npos && lang.find('\0') == std::string::npos &&
           std::find(languages.begin(), languages.end(), lang) != languages.end();
}

std::vector<std::string> RufloIntegration::GetSupportedLanguages() const {
    return {"cpp", "python", "javascript", "typescript", "java", "csharp"};
}

bool RufloIntegration::IsReady() const { return ready; }
void RufloIntegration::SetTimeout(int ms) { if (ms > 0 && ms <= 120000) timeoutMs = ms; }

}
