#include "AssetResourceManager.h"

#include <algorithm>
#include <limits>

namespace NeoEngine {

AssetResourceManager::AssetResourceManager(const AssetRegistry& registry) : registry_(registry) {}

bool AssetResourceManager::Fail(AssetResourceError error) const {
    lastError_ = error;
    return false;
}

uint16_t AssetResourceManager::FindSlot(std::string_view assetId) const {
    for (uint16_t index = 0U; index < kMaxResources; ++index) if (slots_[index].occupied && slots_[index].assetId == assetId) return index;
    return 0xFFFFU;
}

bool AssetResourceManager::ValidHandle(AssetResourceHandle handle) const {
    return handle.slot < kMaxResources && slots_[handle.slot].occupied && slots_[handle.slot].generation == handle.generation;
}

bool AssetResourceManager::BuildDependencyClosure(std::string_view assetId, std::array<std::string, kMaxDependencyClosure>& ids, uint16_t& count, std::array<std::string_view, kMaxDependencyDepth>& path, uint8_t depth) const {
    if (assetId.empty() || depth >= kMaxDependencyDepth) return false;
    for (uint8_t index = 0U; index < depth; ++index) if (path[index] == assetId) return false;
    const AssetDefinition* definition = registry_.Find(assetId);
    if (definition == nullptr || definition->state != AssetState::Ready) return false;
    for (uint16_t index = 0U; index < count; ++index) if (ids[index] == assetId) return true;
    if (count >= kMaxDependencyClosure) return false;
    ids[count++] = std::string(assetId);
    path[depth] = assetId;
    for (const std::string& dependency : definition->dependencies) if (!BuildDependencyClosure(dependency, ids, count, path, static_cast<uint8_t>(depth + 1U))) return false;
    return true;
}

bool AssetResourceManager::RefreshUnleasedSlot(Slot& slot, const AssetDefinition& definition) {
    if (slot.refCount != 0U) return Fail(AssetResourceError::StaleInUse);
    if (slot.generation == std::numeric_limits<uint32_t>::max()) return Fail(AssetResourceError::Capacity);
    slot.state = AssetResourceState::Ready;
    slot.contentHash = definition.contentHash;
    ++slot.generation;
    ++slot.hotReloadGeneration;
    return true;
}

bool AssetResourceManager::FillReceipt(const Slot& slot, AssetResourceReceipt& receipt) const {
    if (!slot.occupied) return false;
    receipt = {slot.assetId, {static_cast<uint16_t>(&slot - slots_.data()), slot.generation}, slot.state, slot.contentHash, slot.refCount, slot.dependencyCount, slot.hotReloadGeneration};
    return true;
}

bool AssetResourceManager::Acquire(std::string_view assetId, AssetResourceHandle& handle) {
    handle = {};
    if (assetId.empty()) return Fail(AssetResourceError::InvalidIdentifier);
    std::array<std::string, kMaxDependencyClosure> closureIds{};
    std::array<std::string_view, kMaxDependencyDepth> path{};
    uint16_t closureCount = 0U;
    if (!BuildDependencyClosure(assetId, closureIds, closureCount, path, 0U)) return Fail(AssetResourceError::MissingDependency);

    std::array<uint16_t, kMaxDependencyClosure> targetSlots{};
    uint16_t missing = 0U;
    for (uint16_t index = 0U; index < closureCount; ++index) {
        targetSlots[index] = FindSlot(closureIds[index]);
        if (targetSlots[index] == 0xFFFFU) ++missing;
    }
    if (activeResourceCount_ > kMaxResources - missing) return Fail(AssetResourceError::Capacity);
    for (uint16_t index = 0U; index < closureCount; ++index) {
        if (targetSlots[index] == 0xFFFFU) continue;
        const Slot& slot = slots_[targetSlots[index]];
        const AssetDefinition* definition = registry_.Find(closureIds[index]);
        if (definition == nullptr || definition->state != AssetState::Ready) return Fail(AssetResourceError::NotReady);
        if (slot.refCount != 0U && (slot.state == AssetResourceState::Stale || slot.contentHash != definition->contentHash)) return Fail(AssetResourceError::StaleInUse);
        if (slot.refCount == std::numeric_limits<uint32_t>::max()) return Fail(AssetResourceError::RefcountOverflow);
    }
    std::array<bool, kMaxResources> reserved{};
    for (uint16_t index = 0U; index < closureCount; ++index) if (targetSlots[index] == 0xFFFFU) {
        for (uint16_t candidate = 0U; candidate < kMaxResources; ++candidate) if (!slots_[candidate].occupied && !reserved[candidate]) { targetSlots[index] = candidate; reserved[candidate] = true; break; }
        if (targetSlots[index] == 0xFFFFU) return Fail(AssetResourceError::Capacity);
    }
    for (uint16_t index = 0U; index < closureCount; ++index) {
        Slot& slot = slots_[targetSlots[index]];
        const AssetDefinition* definition = registry_.Find(closureIds[index]);
        if (definition == nullptr) return Fail(AssetResourceError::MissingDependency);
        if (!slot.occupied) {
            slot = {};
            slot.occupied = true;
            slot.assetId = closureIds[index];
            slot.generation = 1U;
            slot.contentHash = definition->contentHash;
            ++activeResourceCount_;
        } else if (slot.state == AssetResourceState::Stale || slot.contentHash != definition->contentHash) {
            if (!RefreshUnleasedSlot(slot, *definition)) return false;
        }
        slot.state = AssetResourceState::Ready;
        slot.dependencyCount = index == 0U ? static_cast<uint16_t>(closureCount - 1U) : 0U;
        slot.dependencySlots.fill(0xFFFFU);
        if (index == 0U) for (uint16_t dependency = 1U; dependency < closureCount; ++dependency) slot.dependencySlots[dependency - 1U] = targetSlots[dependency];
    }
    for (uint16_t index = 0U; index < closureCount; ++index) {
        Slot& slot = slots_[targetSlots[index]];
        ++slot.refCount;
        ++totalLeaseCount_;
    }
    handle = {targetSlots[0], slots_[targetSlots[0]].generation};
    lastError_ = AssetResourceError::None;
    return true;
}

bool AssetResourceManager::Release(AssetResourceHandle handle) {
    if (!ValidHandle(handle)) return Fail(AssetResourceError::InvalidHandle);
    Slot& root = slots_[handle.slot];
    if (root.refCount == 0U) return Fail(AssetResourceError::RefcountUnderflow);
    if (root.generation == std::numeric_limits<uint32_t>::max()) return Fail(AssetResourceError::Capacity);
    if (totalLeaseCount_ < static_cast<uint32_t>(root.dependencyCount) + 1U) return Fail(AssetResourceError::RefcountUnderflow);
    std::array<uint16_t, kMaxDependencyClosure> targets{};
    const uint16_t targetCount = static_cast<uint16_t>(root.dependencyCount + 1U);
    targets[0] = handle.slot;
    for (uint16_t index = 1U; index < targetCount; ++index) targets[index] = root.dependencySlots[index - 1U];
    for (uint16_t index = 0U; index < targetCount; ++index) if (targets[index] >= kMaxResources || !slots_[targets[index]].occupied || slots_[targets[index]].refCount == 0U) return Fail(AssetResourceError::RefcountUnderflow);
    for (uint16_t index = 0U; index < targetCount; ++index) { --slots_[targets[index]].refCount; --totalLeaseCount_; }
    ++root.generation;
    lastError_ = AssetResourceError::None;
    return true;
}

bool AssetResourceManager::ReloadIfSafe(std::string_view assetId) {
    const uint16_t rootSlot = FindSlot(assetId);
    if (rootSlot == 0xFFFFU) { lastError_ = AssetResourceError::None; return true; }
    if (slots_[rootSlot].refCount != 0U) return Fail(AssetResourceError::StaleInUse);
    std::array<std::string, kMaxDependencyClosure> closureIds{};
    std::array<std::string_view, kMaxDependencyDepth> path{};
    uint16_t closureCount = 0U;
    if (!BuildDependencyClosure(assetId, closureIds, closureCount, path, 0U)) return Fail(AssetResourceError::MissingDependency);
    std::array<uint16_t, kMaxDependencyClosure> targetSlots{};
    for (uint16_t index = 0U; index < closureCount; ++index) {
        targetSlots[index] = FindSlot(closureIds[index]);
        if (targetSlots[index] == 0xFFFFU || slots_[targetSlots[index]].refCount != 0U) return Fail(AssetResourceError::StaleInUse);
    }
    for (uint16_t index = 0U; index < closureCount; ++index) {
        const AssetDefinition* definition = registry_.Find(closureIds[index]);
        if (definition == nullptr || definition->state != AssetState::Ready || !RefreshUnleasedSlot(slots_[targetSlots[index]], *definition)) return Fail(AssetResourceError::HotReloadRejected);
    }
    lastError_ = AssetResourceError::None;
    return true;
}

bool AssetResourceManager::SyncHotReload(std::string_view assetId) {
    const AssetDefinition* definition = registry_.Find(assetId);
    if (definition == nullptr || definition->state != AssetState::Ready) return Fail(AssetResourceError::NotReady);
    const uint16_t slotIndex = FindSlot(assetId);
    if (slotIndex == 0xFFFFU) { lastError_ = AssetResourceError::None; return true; }
    if (slots_[slotIndex].refCount != 0U) { slots_[slotIndex].state = AssetResourceState::Stale; return Fail(AssetResourceError::StaleInUse); }
    slots_[slotIndex].state = AssetResourceState::Stale;
    for (Slot& dependent : slots_) if (dependent.occupied && dependent.refCount == 0U) for (uint16_t index = 0U; index < dependent.dependencyCount; ++index) if (dependent.dependencySlots[index] == slotIndex) dependent.state = AssetResourceState::Stale;
    return ReloadIfSafe(assetId);
}

bool AssetResourceManager::Query(AssetResourceHandle handle, AssetResourceReceipt& receipt) const {
    if (!ValidHandle(handle)) return Fail(AssetResourceError::InvalidHandle);
    return FillReceipt(slots_[handle.slot], receipt);
}

bool AssetResourceManager::Query(std::string_view assetId, AssetResourceReceipt& receipt) const {
    const uint16_t slot = FindSlot(assetId);
    if (slot == 0xFFFFU) return Fail(AssetResourceError::InvalidIdentifier);
    return FillReceipt(slots_[slot], receipt);
}

const std::vector<uint8_t>* AssetResourceManager::Data(AssetResourceHandle handle) const {
    if (!ValidHandle(handle) || slots_[handle.slot].state != AssetResourceState::Ready) return nullptr;
    return registry_.Data(slots_[handle.slot].assetId);
}

} // namespace NeoEngine
