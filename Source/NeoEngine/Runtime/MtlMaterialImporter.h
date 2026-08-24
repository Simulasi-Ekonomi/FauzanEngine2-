#pragma once

#include "MeshRenderer.h"

#include <cstdint>
#include <string_view>

namespace NeoEngine {
enum class MtlMaterialImportError : uint8_t { None, EmptySource, SourceTooLarge, InvalidName, ParseFailed, MissingMaterial, DuplicateMaterial, InvalidValue };
class MtlMaterialImporter {
public:
    static constexpr size_t kMaxSourceBytes = 1U << 20U;
    bool Import(std::string_view source,std::string_view materialName,MeshMaterial& material);
    [[nodiscard]] MtlMaterialImportError LastError() const { return lastError_; }
private:
    MtlMaterialImportError lastError_ = MtlMaterialImportError::None;
};
} // namespace NeoEngine
