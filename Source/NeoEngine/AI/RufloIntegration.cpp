#include "RufloIntegration.h"
#include <curl/curl.h>
#include <json/json.h>
#include <android/log.h>

#define LOG_TAG "RufloIntegration"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

RufloIntegration::RufloIntegration() : ready(false), contextType(ExecutionContextType::Sandbox), runtimeHandle(nullptr), timeoutMs(5000) {}
RufloIntegration::~RufloIntegration() { Shutdown(); }

bool RufloIntegration::Initialize(ExecutionContextType ctx) {
    contextType = ctx;
    // Cek koneksi ke server Ruflo (localhost:5000)
    CURL* curl = curl_easy_init();
    if (!curl) { ready = false; return false; }
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:5000/health");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    ready = (res == CURLE_OK);
    if (ready) LOGI("Ruflo server connected");
    else LOGI("Ruflo server not available");
    return ready;
}

void RufloIntegration::Shutdown() { ready = false; }

ExecutionResult RufloIntegration::ExecuteCode(const std::string& code, const std::string& lang) {
    if (!ready) return {1, "", "Ruflo not initialized", 0.0f, false};

    CURL* curl = curl_easy_init();
    if (!curl) return {1, "", "CURL init failed", 0.0f, false};

    Json::Value body;
    body["code"] = code;
    body["language"] = lang;
    Json::FastWriter writer;
    std::string jsonBody = writer.write(body);

    std::string responseStr;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:5000/execute");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStr);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutMs / 1000);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return {1, "", "Ruflo request failed", 0.0f, false};
    }

    Json::Value root;
    Json::Reader reader;
    if (reader.parse(responseStr, root)) {
        ExecutionResult result;
        result.exitCode = root.get("exitCode", 0).asInt();
        result.stdout = root.get("stdout", "").asString();
        result.stderr = root.get("stderr", "").asString();
        result.executionTime = root.get("executionTime", 0.0f).asFloat();
        result.success = root.get("success", false).asBool();
        return result;
    }

    return {1, "", "Invalid response from Ruflo", 0.0f, false};
}

ExecutionResult RufloIntegration::ExecuteWithEnvironment(const std::string& code, const std::string& lang,
                                                         const std::map<std::string, std::string>& env) {
    return ExecuteCode(code, lang);
}

bool RufloIntegration::ValidateCode(const std::string& code, const std::string& lang) {
    return ready && !code.empty();
}

std::vector<std::string> RufloIntegration::GetSupportedLanguages() const {
    return {"cpp", "python", "javascript", "typescript", "java", "csharp"};
}

bool RufloIntegration::IsReady() const { return ready; }
void RufloIntegration::SetTimeout(int ms) { timeoutMs = ms; }

}
