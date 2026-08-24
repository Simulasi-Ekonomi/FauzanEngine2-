#pragma once
#include <string>
#include <vector>
#include <memory>

namespace NeoEngine {

struct HermesEngineResponse {
    std::string text;
    float confidence;
    int tokenCount;
    std::string modelInfo;
};

class HermesEngineIntegration {
public:
    HermesEngineIntegration();
    ~HermesEngineIntegration();

    // Default server diubah ke 11434 (Ollama)
    bool Initialize(const std::string& modelPath = "",
                    const std::string& serverAddress = "http://localhost:11434");
    void Shutdown();

    HermesEngineResponse GenerateText(const std::string& prompt, float temperature = 0.7f, int maxTokens = 2048);
    HermesEngineResponse Chat(const std::string& message, int contextSize = 4096);
    HermesEngineResponse CodeGeneration(const std::string& description);

    bool IsReady() const { return m_Ready; }
    std::string GetModelInfo() const;

private:
    bool m_Ready = false;
    std::string m_ModelPath;
    std::string m_ServerAddress;
    void* m_ModelHandle = nullptr;

    std::string HttpPost(const std::string& endpoint, const std::string& jsonBody);
};

} // namespace NeoEngine
