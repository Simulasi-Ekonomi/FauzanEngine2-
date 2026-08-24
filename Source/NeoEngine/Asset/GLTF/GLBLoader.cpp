#include <cassert>
#include "GLBLoader.h"
#include <fstream>

bool GLBLoader::Load(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open())
        return false;

    uint32_t magic   = 0;
    uint32_t version = 0;
    uint32_t length  = 0;

    // FIX: reinterpret_cast eksplisit untuk binary read
    file.read(reinterpret_cast<char*>(&magic),   sizeof(magic));
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    file.read(reinterpret_cast<char*>(&length),  sizeof(length));

    while (file.tellg() < static_cast<std::streampos>(length))
    {
        GLBChunk chunk;

        file.read(reinterpret_cast<char*>(&chunk.length), sizeof(chunk.length));
        file.read(reinterpret_cast<char*>(&chunk.type),   sizeof(chunk.type));

        chunk.data.resize(chunk.length);
        file.read(reinterpret_cast<char*>(chunk.data.data()), chunk.length);

        if (chunk.type == 0x4E4F534A)  // JSON
        {
            jsonChunk = std::string(chunk.data.begin(), chunk.data.end());
        }
        else if (chunk.type == 0x004E4942)  // BIN
        {
            binaryChunk = std::move(chunk.data);  // FIX: move daripada copy
        }
    }

    return true;
}

const std::string& GLBLoader::GetJSON() const   { return jsonChunk; }
const std::vector<uint8_t>& GLBLoader::GetBinary() const { return binaryChunk; }
