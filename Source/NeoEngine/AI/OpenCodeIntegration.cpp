#include "OpenCodeIntegration.h"
#include <curl/curl.h>
#include <json/json.h>
#include <android/log.h>
#include <limits>

#define LOG_TAG_OC "OpenCodeIntegration"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_OC, __VA_ARGS__)

namespace NeoEngine {

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    if (output == nullptr || contents == nullptr || (nmemb != 0U && size > std::numeric_limits<size_t>::max() / nmemb)) return 0U;
    const size_t totalSize = size * nmemb;
    if (totalSize > 16U * 1024U * 1024U || output->size() > 16U * 1024U * 1024U - totalSize) return 0U;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

OpenCodeIntegration::OpenCodeIntegration() : ready(false) {}
OpenCodeIntegration::~OpenCodeIntegration() { Shutdown(); }

bool OpenCodeIntegration::Initialize() {
    CURL* curl = curl_easy_init();
    if (!curl) { ready = false; return false; }
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:5000/health");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
    const CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    ready = (res == CURLE_OK);
    LOGI("OpenCode integration %s", ready ? "connected" : "unavailable");
    return ready;
}

void OpenCodeIntegration::Shutdown() { ready = false; }

GeneratedCode OpenCodeIntegration::GenerateFromDescription(const std::string& desc) {
    GeneratedCode gc{};
    if (!ready || desc.empty()) return gc;
    CURL* curl = curl_easy_init();
    if (!curl) return gc;

    Json::Value body;
    body["prompt"] = "Generate C++ game engine code for: " + desc;
    body["max_tokens"] = 2048;
    Json::FastWriter writer;
    const std::string jsonBody = writer.write(body);
    std::string responseStr;
    struct curl_slist* headers = curl_slist_append(nullptr, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8765/v1/chat/completions");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStr);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    const CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK || responseStr.empty()) return gc;

    gc.language = "cpp";
    gc.code = responseStr;
    gc.description = desc;
    gc.complexity = 5;
    return gc;
}

GeneratedCode OpenCodeIntegration::GenerateFromTemplate(const std::string& tmpl,
    const std::map<std::string, std::string>& params) {
    GeneratedCode gc{};
    if (!ready || tmpl.empty()) return gc;
    gc.language = "cpp";
    gc.code = tmpl;
    for (const auto& [k, v] : params) {
        if (k.empty()) return GeneratedCode{};
        gc.code += "\n// " + k + " = " + v;
    }
    gc.description = "From template " + tmpl;
    gc.complexity = 2;
    return gc;
}

std::vector<std::string> OpenCodeIntegration::GetSupportedLanguages() const {
    return {"cpp", "python", "javascript", "typescript", "java", "csharp"};
}

bool OpenCodeIntegration::ValidateCode(const GeneratedCode& code) {
    return ready && !code.language.empty() && !code.code.empty() && !code.description.empty() &&
           code.complexity >= 0 && code.complexity <= 10;
}

bool OpenCodeIntegration::IsReady() const { return ready; }

}
