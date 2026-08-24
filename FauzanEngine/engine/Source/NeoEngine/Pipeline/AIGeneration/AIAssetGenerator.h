#pragma once
#include <vector>
#include <string>

class AIAssetGenerator
{
public:

    void AddPrompt(const std::string& prompt);
    void Generate();

private:

    std::vector<std::string> prompts;
};
