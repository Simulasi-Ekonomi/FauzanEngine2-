#include "HermesEngineIntegration.h"
#include <iostream>

int main() {
    NeoEngine::HermesEngineIntegration testAgent;
    if(testAgent.Initialize()) {
        std::cout << "SUCCESS: C++ Bridge can see Ollama on port 11434" << std::endl;
        std::cout << testAgent.GetModelInfo() << std::endl;
    } else {
        std::cerr << "FAILED: Check if 'ollama serve' is running!" << std::endl;
    }
    return 0;
}
