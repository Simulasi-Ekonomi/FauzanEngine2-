#pragma once

#include "MeshRenderer.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace NeoEngine {
enum class ObjMeshImportError : uint8_t { None, EmptySource, SourceTooLarge, ParseFailed, UnsupportedPrimitive, EmptyMesh, InvalidAttribute, MissingNormal, DegenerateTriangle, Capacity };
struct ObjMeshImportOptions { bool generateFlatNormals = false; };
class ObjMeshImporter {
public:
    static constexpr size_t kMaxSourceBytes = 1U << 20U;
    bool Import(std::string_view source,std::vector<MeshVertex>& vertices,std::vector<uint16_t>& indices,const ObjMeshImportOptions& options = {});
    [[nodiscard]] ObjMeshImportError LastError() const { return lastError_; }
private:
    ObjMeshImportError lastError_ = ObjMeshImportError::None;
};
} // namespace NeoEngine
