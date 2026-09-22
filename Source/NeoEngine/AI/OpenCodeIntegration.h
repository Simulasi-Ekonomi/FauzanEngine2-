#pragma once
#include <string>
#include <vector>
#include <map>

namespace NeoEngine {

struct GeneratedCode {
    std::string language;
    std::string code;
    std::string description;
    int complexity = 0;
};

class OpenCodeIntegration {
public:
    OpenCodeIntegration();
    ~OpenCodeIntegration();

    bool Initialize();
    void Shutdown();

    GeneratedCode GenerateFromDescription(const std::string& description);
    GeneratedCode GenerateFromTemplate(const std::string& templateName,
                                       const std::map<std::string, std::string>& parameters);
    std::vector<std::string> GetSupportedLanguages() const;
    bool ValidateCode(const GeneratedCode& code);
    bool IsReady() const;

private:
    bool ready = false;
    void* generatorHandle = nullptr;
    std::string healthUrl;
    std::string completionUrl;
};

}
