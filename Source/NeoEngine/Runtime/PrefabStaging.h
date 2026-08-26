#pragma once

#include "Runtime/AssetRegistry.h"
#include "Runtime/EditorScenePrefabCodec.h"

#include <cstdint>
#include <deque>
#include <string>
#include <string_view>

namespace NeoEngine {

enum class PrefabStagingError : uint8_t { None, MissingAsset, WrongKind, AssetNotReady, DuplicateResource, DecodeFailed, CapacityExceeded };

struct CpuPrefabResource {
    std::string assetId;
    uint64_t sourceHash = 0U;
    EditorScenePrefab prefab{};
};

// Bounded ready-asset snapshot for prefab document data. It owns no external
// asset lifetime and does not perform filesystem observation or spawning.
class PrefabStagingStore {
public:
    static constexpr size_t kMaxPrefabs = 64U;
    static constexpr size_t kMaxStoredActors = EditorSceneDocumentAdapter::kMaxActors;

    bool Stage(const AssetRegistry& registry, std::string_view assetId);
    bool Refresh(const AssetRegistry& registry, std::string_view assetId);
    [[nodiscard]] bool CanRefresh(const AssetRegistry& registry, std::string_view assetId) const;
    [[nodiscard]] const CpuPrefabResource* Find(std::string_view assetId) const;
    [[nodiscard]] bool IsCurrent(const AssetRegistry& registry, std::string_view assetId) const;
    [[nodiscard]] size_t ResourceCount() const { return resources_.size(); }
    [[nodiscard]] size_t StagedActors() const { return stagedActors_; }
    [[nodiscard]] PrefabStagingError LastError() const { return lastError_; }

private:
    [[nodiscard]] bool DecodeReady(const AssetRegistry& registry, std::string_view assetId, EditorScenePrefab& prefab, uint64_t& sourceHash, PrefabStagingError& error) const;
    std::deque<CpuPrefabResource> resources_;
    size_t stagedActors_ = 0U;
    PrefabStagingError lastError_ = PrefabStagingError::None;
};

} // namespace NeoEngine
