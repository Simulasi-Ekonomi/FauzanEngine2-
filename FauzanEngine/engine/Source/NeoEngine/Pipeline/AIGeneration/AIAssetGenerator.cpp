#include "AIAssetGenerator.h"
#include <iostream>

void AIAssetGenerator::AddPrompt(const std::string& prompt)
{
    prompts.push_back(prompt);
}

void AIAssetGenerator::Generate()
{
    for(const auto& p : prompts)
    {
        std::cout<<"Generating asset from prompt: "<<p<<"\n";
    }
}
