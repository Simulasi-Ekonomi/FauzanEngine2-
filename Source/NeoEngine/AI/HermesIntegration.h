#pragma once
#include <string>

namespace NeoEngine {

enum class HermesModelType { Small, Medium, Large };

struct HermesResponse {
    std::string text;
    float confidence;
    int tokenCount;
    std::string modelUsed;
};

class HermesIntegration {
public:
    HermesIntegration();
    ~HermesIntegration();

    bool Initialize(HermesModelType modelType = HermesModelType::Medium);
    void Shutdown();
    
    HermesResponse GenerateText(const std::string& prompt, float temperature = 0.7f);
    HermesResponse Chat(const std::string& message, int contextSize = 2048);
    HermesResponse CodeGeneration(const std::string& description);
    
    void SetTemperature(float temp);
    void SetMaxTokens(int maxTokens);
    
    bool IsReady() const;
    std::string GetModelInfo() const;

private:
    bool ready;
    HermesModelType currentModel;
    void* modelHandle;
    float temperature;
    int maxTokens;
    std::string hermesEndpoint;   // <-- tambahan untuk HTTP mode
};

}
