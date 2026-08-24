#pragma once

#include "AssetRegistry.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace NeoEngine {
enum class AssetReloadDiagnosticsError : uint8_t { None, MissingAsset, DependencyGraphInvalid, Capacity };
class AssetReloadDiagnostics {
public:
    bool BuildPlan(const AssetRegistry& registry, std::string_view changedId);
    [[nodiscard]] const std::vector<std::string>& AffectedIds() const { return affectedIds_; }
    [[nodiscard]] AssetReloadDiagnosticsError LastError() const { return lastError_; }
private:
    std::vector<std::string> affectedIds_;
    AssetReloadDiagnosticsError lastError_ = AssetReloadDiagnosticsError::None;
};
} // namespace NeoEngine
