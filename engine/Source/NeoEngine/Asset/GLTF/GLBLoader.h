#include <cassert>
#pragma once
#include <vector>
#include <string>
#include <cstdint>

struct GLBChunk
{
    uint32_t length;
    uint32_t type;
    [[maybe_unused]] std::vector<uint8_t> data;
};

class GLBLoader
{
public:

    bool Load(const std::string& path);

    const std::string& GetJSON() const;
    const std::vector<uint8_t>& GetBinary() const;

private:

    std::string jsonChunk;
    [[maybe_unused]] std::vector<uint8_t> binaryChunk;
};
