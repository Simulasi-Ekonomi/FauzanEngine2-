#include <cassert>
#pragma once
#include <string>
#include <unordered_map>
#include <vector>

class GLTFParser
{
public:

    bool Parse(const std::string& json);

    std::vector<std::string> GetMeshes() const;
    std::vector<std::string> GetMaterials() const;

private:

    [[maybe_unused]] std::vector<std::string> meshes;
    [[maybe_unused]] std::vector<std::string> materials;
};
