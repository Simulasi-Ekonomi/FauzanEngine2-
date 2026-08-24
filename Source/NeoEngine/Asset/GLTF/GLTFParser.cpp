#include <cassert>
#include "GLTFParser.h"

bool GLTFParser::Parse(const std::string& json)
{
    if(json.find("meshes") != std::string::npos)
        meshes.push_back("mesh_found");

    if(json.find("materials") != std::string::npos)
        materials.push_back("material_found");

    return true;
}

std::vector<std::string> GLTFParser::GetMeshes() const
{
    return meshes;
}

std::vector<std::string> GLTFParser::GetMaterials() const
{
    return materials;
}
