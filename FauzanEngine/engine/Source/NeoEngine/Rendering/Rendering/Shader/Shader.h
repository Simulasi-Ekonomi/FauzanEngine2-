#pragma once
#include <string>

class Shader {
public:
    virtual ~Shader() = default;
    virtual void Compile(const std::string& Source) = 0;
    virtual void Bind() = 0;
};
