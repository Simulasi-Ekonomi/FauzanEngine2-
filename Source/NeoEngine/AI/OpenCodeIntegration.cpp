#include "OpenCodeIntegration.h"
#include <curl/curl.h>
#include <json/json.h>
#include <cstdlib>
#include <algorithm>
#include <cctype>

namespace NeoEngine {
namespace {
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    if (!contents || !output) return 0;
    const size_t total = size * nmemb;
    output->append(static_cast<const char*>(contents), total);
    return total;
}

std::string EnvOr(const char* name, const char* fallback) {
    const char* value = std::getenv(name);
    return value && *value ? value : fallback;
}
}

OpenCodeIntegration::OpenCodeIntegration() = default;
OpenCodeIntegration::~OpenCodeIntegration() { Shutdown(); }

bool OpenCodeIntegration::Initialize() {
    healthUrl = EnvOr("NEO_OPENCODE_HEALTH_URL", "http://127.0.0.1:5000/health");
    completionUrl = EnvOr("NEO_OPENCODE_COMPLETION_URL", "http://127.0.0.1:8765/v1/chat/completions");

    CURL* curl = curl_easy_init();
    if (!curl) return false;
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, healthUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 1000L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 2500L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    const CURLcode result = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_cleanup(curl);
    ready = result == CURLE_OK && status >= 200 && status < 300;
    return ready;
}

void OpenCodeIntegration::Shutdown() {
    ready = false;
    generatorHandle = nullptr;
}

GeneratedCode OpenCodeIntegration::GenerateFromDescription(const std::string& description) {
    GeneratedCode out;
    out.language = "cpp";
    out.description = description;
    if (description.empty() || !ready) return out;

    Json::Value request;
    request["prompt"] = "Generate production-safe C++ engine code. Return code only. Requirement: " + description;
    request["max_tokens"] = 4096;
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    const std::string payload = Json::writeString(writer, request);

    CURL* curl = curl_easy_init();
    if (!curl) return out;
    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, completionUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payload.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 1500L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 60000L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    const CURLcode result = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (result != CURLE_OK) return out;

    Json::CharReaderBuilder reader;
    Json::Value root;
    std::string errors;
    std::unique_ptr<Json::CharReader> parser(reader.newCharReader());
    if (!parser->parse(response.data(), response.data() + response.size(), &root, &errors)) return out;

    const Json::Value choices = root["choices"];
    if (!choices.isArray() || choices.empty() || !choices[0]["message"]["content"].isString()) return out;
    out.code = choices[0]["message"]["content"].asString();
    out.complexity = std::max(1, static_cast<int>(out.code.size() / 400));
    if (!ValidateCode(out)) out.code.clear();
    return out;
}

GeneratedCode OpenCodeIntegration::GenerateFromTemplate(
    const std::string& templateName, const std::map<std::string, std::string>& parameters) {
    GeneratedCode out;
    out.language = "cpp";
    out.description = "Template: " + templateName;

    auto get = [&](const char* key, const char* fallback) {
        const auto it = parameters.find(key);
        return it == parameters.end() ? std::string(fallback) : it->second;
    };

    if (templateName == "component") {
        const std::string name = get("name", "GeneratedComponent");
        out.code = "#pragma once\nstruct " + name + " {\n    float x = 0.0f;\n    float y = 0.0f;\n};\n";
    } else if (templateName == "system") {
        const std::string name = get("name", "GeneratedSystem");
        out.code = "#pragma once\nclass " + name + " {\npublic:\n    void Tick(float deltaSeconds) { if (deltaSeconds > 0.0f) accumulator_ += deltaSeconds; }\n    float Accumulator() const { return accumulator_; }\nprivate:\n    float accumulator_ = 0.0f;\n};\n";
    } else if (templateName == "enum") {
        const std::string name = get("name", "GeneratedState");
        out.code = "#pragma once\nenum class " + name + " : unsigned char { Invalid, Ready, Active, Failed };\n";
    } else {
        return out;
    }

    out.complexity = 1;
    return out;
}

std::vector<std::string> OpenCodeIntegration::GetSupportedLanguages() const {
    return {"cpp", "python", "javascript", "typescript", "java", "csharp"};
}

bool OpenCodeIntegration::ValidateCode(const GeneratedCode& code) {
    if (code.language.empty() || code.code.size() < 20) return false;
    if (code.code.find("TODO") != std::string::npos ||
        code.code.find("not implemented") != std::string::npos ||
        code.code.find("IMPLEMENT_ME") != std::string::npos) return false;
    if (code.code.find("GeneratedFeature()") != std::string::npos) return false;
    return true;
}

bool OpenCodeIntegration::IsReady() const { return ready; }

} // namespace NeoEngine
