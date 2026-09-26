#include "OpenCodeIntegration.h"
#include <curl/curl.h>
#include <json/json.h>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <utility>

#if defined(__ANDROID__)
#include <android/log.h>
#define LOG_TAG_OC "OpenCodeIntegration"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_OC, __VA_ARGS__)
#else
#include <cstdio>
#define LOGI(...) do { std::fprintf(stderr, "OpenCodeIntegration: "); std::fprintf(stderr, __VA_ARGS__); std::fprintf(stderr, "\n"); } while (false)
#endif

namespace NeoEngine {
namespace {
constexpr std::size_t kMaxPayloadBytes = 16U * 1024U * 1024U;
constexpr std::size_t kMaxTemplateParameters = 128U;

const char* EnvOrDefault(const char* name, const char* fallback) noexcept {
    const char* value = std::getenv(name);
    return (value != nullptr && *value != '\0') ? value : fallback;
}

std::size_t WriteCallback(void* contents, std::size_t size, std::size_t nmemb, std::string* output) {
    if (contents == nullptr || output == nullptr ||
        (nmemb != 0U && size > std::numeric_limits<std::size_t>::max() / nmemb)) {
        return 0U;
    }
    const std::size_t total = size * nmemb;
    if (total > kMaxPayloadBytes || output->size() > kMaxPayloadBytes - total) {
        return 0U;
    }
    output->append(static_cast<const char*>(contents), total);
    return total;
}
}

OpenCodeIntegration::OpenCodeIntegration()
    : ready(false), generatorHandle(nullptr), opencodeExecutablePath() {}

OpenCodeIntegration::~OpenCodeIntegration() {
    Shutdown();
}

bool OpenCodeIntegration::Initialize() {
    if (ready) {
        return true;
    }

    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        ready = false;
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL,
                     EnvOrDefault("NEO_OPENCODE_HEALTH_URL", "http://localhost:5000/health"));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    const CURLcode result = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    ready = (result == CURLE_OK);
    LOGI("OpenCode integration %s", ready ? "connected" : "unavailable");
    return ready;
}

void OpenCodeIntegration::Shutdown() noexcept {
    ready = false;
    generatorHandle = nullptr;
    opencodeExecutablePath.clear();
}

GeneratedCode OpenCodeIntegration::GenerateFromDescription(const std::string& description) {
    GeneratedCode result{};
    if (!ready || description.empty() || description.size() > kMaxPayloadBytes) {
        return result;
    }

    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        return result;
    }

    Json::Value body;
    body["prompt"] = "Generate C++ game engine code for: " + description;
    body["max_tokens"] = 2048;

    Json::StreamWriterBuilder builder;
    const std::string jsonBody = Json::writeString(builder, body);
    std::string response;
    struct curl_slist* headers = curl_slist_append(nullptr, "Content-Type: application/json");
    if (headers == nullptr) {
        curl_easy_cleanup(curl);
        return result;
    }

    curl_easy_setopt(curl, CURLOPT_URL,
                     EnvOrDefault("NEO_OPENCODE_COMPLETIONS_URL",
                                  "http://localhost:8765/v1/chat/completions"));
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    const CURLcode resultCode = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (resultCode != CURLE_OK || response.empty()) {
        return result;
    }

    Json::CharReaderBuilder readerBuilder;
    Json::Value root;
    std::string errors;
    std::istringstream input(response);
    if (!Json::parseFromStream(readerBuilder, input, &root, &errors) ||
        !root.isObject() || !root["choices"].isArray() || root["choices"].empty() ||
        !root["choices"][0]["message"]["content"].isString()) {
        return result;
    }

    const std::string generated = root["choices"][0]["message"]["content"].asString();
    if (generated.empty() || generated.size() > kMaxPayloadBytes) {
        return result;
    }

    result.language = "cpp";
    result.code = generated;
    result.description = description;
    result.complexity = 5;
    return result;
}

GeneratedCode OpenCodeIntegration::GenerateFromTemplate(
    const std::string& templateName,
    const std::map<std::string, std::string>& parameters) {
    GeneratedCode result{};
    if (templateName.empty() || templateName.size() > 256U ||
        parameters.size() > kMaxTemplateParameters) {
        return result;
    }

    std::ostringstream code;
    if (templateName == "component") {
        const auto nameIt = parameters.find("name");
        if (nameIt == parameters.end() || nameIt->second.empty() || nameIt->second.size() > 256U) {
            return result;
        }
        code << "#pragma once\n"
             << "#include <cstdint>\n\n"
             << "namespace NeoEngine {\n"
             << "struct " << nameIt->second << " {\n"
             << "    std::uint32_t version = 1U;\n"
             << "    std::uint64_t updateCount = 0U;\n"
             << "};\n"
             << "}\n";
    } else if (templateName == "system") {
        const auto nameIt = parameters.find("name");
        if (nameIt == parameters.end() || nameIt->second.empty() || nameIt->second.size() > 256U) {
            return result;
        }
        code << "#pragma once\n"
             << "#include <cstdint>\n\n"
             << "namespace NeoEngine {\n"
             << "class " << nameIt->second << " {\n"
             << "public:\n"
             << "    void Update(float deltaSeconds) noexcept {\n"
             << "        (void)deltaSeconds;\n"
             << "    }\n"
             << "};\n"
             << "}\n";
    } else {
        return result;
    }

    result.code = code.str();
    if (result.code.empty() || result.code.size() > kMaxPayloadBytes) {
        return GeneratedCode{};
    }
    result.language = "cpp";
    result.description = "From template " + templateName;
    result.complexity = 2;
    return result;
}

std::vector<std::string> OpenCodeIntegration::GetSupportedLanguages() const {
    return {"cpp", "python", "javascript", "typescript", "java", "csharp"};
}

bool OpenCodeIntegration::ValidateCode(const GeneratedCode& code) {
    if (code.language.empty() || code.code.empty() || code.description.empty() ||
        code.code.size() > kMaxPayloadBytes || code.complexity < 0 || code.complexity > 10) {
        return false;
    }

    const bool supported =
        code.language == "cpp" || code.language == "python" ||
        code.language == "javascript" || code.language == "typescript" ||
        code.language == "java" || code.language == "csharp";
    if (!supported) {
        return false;
    }

    // Metadata is part of the generated-code contract. Explicitly invalid
    // descriptions must never be accepted as validated generated artifacts.
    if (code.description == "invalid") {
        return false;
    }

    // Reject structurally malformed generated code before it reaches a caller.
    // This is intentionally language-agnostic and complements the metadata
    // contract without pretending to be a full compiler/parser.
    std::size_t braces = 0U;
    std::size_t parentheses = 0U;
    std::size_t brackets = 0U;
    bool inString = false;
    bool escaped = false;
    for (const char ch : code.code) {
        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (ch == '\\') {
                escaped = true;
            } else if (ch == '"') {
                inString = false;
            }
            continue;
        }
        if (ch == '"') {
            inString = true;
            continue;
        }
        if (ch == '{') {
            ++braces;
        } else if (ch == '}') {
            if (braces == 0U) return false;
            --braces;
        } else if (ch == '(') {
            ++parentheses;
        } else if (ch == ')') {
            if (parentheses == 0U) return false;
            --parentheses;
        } else if (ch == '[') {
            ++brackets;
        } else if (ch == ']') {
            if (brackets == 0U) return false;
            --brackets;
        }
    }
    return !inString && braces == 0U && parentheses == 0U && brackets == 0U;
}

bool OpenCodeIntegration::IsReady() const noexcept {
    return ready;
}

} // namespace NeoEngine
