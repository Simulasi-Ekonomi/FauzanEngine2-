#pragma once
#include <string>
#include <vector>

namespace NeoEngine {

enum class Gemma4ModelSize { Small, Base, Large };

struct Gemma4Response {
    std::string generatedText;
    float confidence;
    std::vector<float> embeddings;
    int tokensUsed;
};

class Gemma4Integration {
public:
    Gemma4Integration();
    ~Gemma4Integration();

    bool Initialize(Gemma4ModelSize size = Gemma4ModelSize::Base);
    void Shutdown();
    
    Gemma4Response GenerateText(const std::string& prompt, int maxLength = 512);
    std::vector<float> GetEmbeddings(const std::string& text);
    Gemma4Response Summarize(const std::string& text);
    
    bool IsReady() const;
    std::string GetModelInfo() const;
    void SetModelSize(Gemma4ModelSize size);

private:
    std::string GetDefaultModelPath() const;
    bool ready;
    Gemma4ModelSize modelSize;
    void* modelHandle;
    std::string modelFilePath;
};

}
