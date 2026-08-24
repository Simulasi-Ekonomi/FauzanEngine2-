#pragma once
#include <string>
#include <cstdint>

class Object {
public:
    Object();
    virtual ~Object();
    uint64_t GetID() const;
    std::string GetName() const;
protected:
    uint64_t ID;
    std::string Name;
};
