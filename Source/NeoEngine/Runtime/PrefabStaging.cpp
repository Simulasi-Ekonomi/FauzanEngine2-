#include "Runtime/PrefabStaging.h"

#include <algorithm>

namespace NeoEngine {
bool PrefabStagingStore::DecodeReady(const AssetRegistry& registry, std::string_view assetId, EditorScenePrefab& prefab, uint64_t& sourceHash, PrefabStagingError& error) const {
    const AssetDefinition* definition = registry.Find(assetId);
    const std::vector<uint8_t>* bytes = registry.Data(assetId);
    if (definition == nullptr || bytes == nullptr) { error = PrefabStagingError::MissingAsset; return false; }
    if (definition->kind != AssetKind::Prefab) { error = PrefabStagingError::WrongKind; return false; }
    if (definition->state != AssetState::Ready) { error = PrefabStagingError::AssetNotReady; return false; }
    EditorScenePrefabCodec codec;
    if (!codec.Decode(*bytes, prefab)) { error = PrefabStagingError::DecodeFailed; return false; }
    sourceHash = definition->contentHash;
    error = PrefabStagingError::None;
    return true;
}

bool PrefabStagingStore::Stage(const AssetRegistry& registry, std::string_view assetId) {
    if (Find(assetId) != nullptr) { lastError_ = PrefabStagingError::DuplicateResource; return false; }
    if (resources_.size() >= kMaxPrefabs) { lastError_ = PrefabStagingError::CapacityExceeded; return false; }
    EditorScenePrefab prefab{};
    uint64_t sourceHash = 0U;
    PrefabStagingError error = PrefabStagingError::None;
    if (!DecodeReady(registry, assetId, prefab, sourceHash, error)) { lastError_ = error; return false; }
    if (prefab.actors.size() > kMaxStoredActors - stagedActors_) { lastError_ = PrefabStagingError::CapacityExceeded; return false; }
    const size_t actorCount = prefab.actors.size();
    resources_.push_back({std::string(assetId), sourceHash, std::move(prefab)});
    stagedActors_ += actorCount;
    lastError_ = PrefabStagingError::None;
    return true;
}

bool PrefabStagingStore::Refresh(const AssetRegistry& registry, std::string_view assetId) {
    const auto found = std::find_if(resources_.begin(), resources_.end(), [assetId](const CpuPrefabResource& resource) { return resource.assetId == assetId; });
    if (found == resources_.end()) { lastError_ = PrefabStagingError::MissingAsset; return false; }
    EditorScenePrefab prefab{};
    uint64_t sourceHash = 0U;
    PrefabStagingError error = PrefabStagingError::None;
    if (!DecodeReady(registry, assetId, prefab, sourceHash, error)) { lastError_ = error; return false; }
    const size_t retainedActors = stagedActors_ - found->prefab.actors.size();
    if (prefab.actors.size() > kMaxStoredActors - retainedActors) { lastError_ = PrefabStagingError::CapacityExceeded; return false; }
    const size_t replacementActors = prefab.actors.size();
    found->sourceHash = sourceHash;
    found->prefab = std::move(prefab);
    stagedActors_ = retainedActors + replacementActors;
    lastError_ = PrefabStagingError::None;
    return true;
}

bool PrefabStagingStore::CanRefresh(const AssetRegistry& registry, std::string_view assetId) const {
    const CpuPrefabResource* found = Find(assetId);
    if (found == nullptr) return false;
    EditorScenePrefab prefab{};
    uint64_t sourceHash = 0U;
    PrefabStagingError error = PrefabStagingError::None;
    if (!DecodeReady(registry, assetId, prefab, sourceHash, error)) return false;
    const size_t retainedActors = stagedActors_ - found->prefab.actors.size();
    return prefab.actors.size() <= kMaxStoredActors - retainedActors;
}

const CpuPrefabResource* PrefabStagingStore::Find(std::string_view assetId) const {
    const auto found = std::find_if(resources_.begin(), resources_.end(), [assetId](const CpuPrefabResource& resource) { return resource.assetId == assetId; });
    return found == resources_.end() ? nullptr : &*found;
}

bool PrefabStagingStore::IsCurrent(const AssetRegistry& registry, std::string_view assetId) const {
    const CpuPrefabResource* resource = Find(assetId);
    const AssetDefinition* definition = registry.Find(assetId);
    return resource != nullptr && definition != nullptr && definition->kind == AssetKind::Prefab && definition->state == AssetState::Ready && definition->contentHash == resource->sourceHash;
}

} // namespace NeoEngine
