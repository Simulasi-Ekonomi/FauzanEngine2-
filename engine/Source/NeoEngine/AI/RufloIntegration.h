#pragma once
#include <string>
#include <vector>
#include <map>

namespace NeoEngine {

enum class ExecutionContextType { Sandbox, Native, VM };

struct ExecutionResult {
    int exitCode;
    std::string stdout;
    std::string stderr;
    float executionTime;
    bool success;
};

class RufloIntegration {
public:
    RufloIntegration();
    ~RufloIntegration();

    bool Initialize(ExecutionContextType contextType = ExecutionContextType::Sandbox);
    void Shutdown();
    
    ExecutionResult ExecuteCode(const std::string& code, const std::string& language);
    ExecutionResult ExecuteWithEnvironment(const std::string& code, const std::string& language,
                                          const std::map<std::string, std::string>& env);
    bool ValidateCode(const std::string& code, const std::string& language);
    std::vector<std::string> GetSupportedLanguages() const;
    bool IsReady() const;
    void SetTimeout(int milliseconds);

private:
    bool ready;
    ExecutionContextType contextType;
    void* runtimeHandle;
    int timeoutMs;
};

}
