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
    if (handle.slot >= kMaxLeases) return false;
    const LeaseSlot& lease = leases_[handle.slot];
    return lease.occupied && lease.generation == handle.generation && lease.rootResourceSlot < kMaxResources && slots_[lease.rootResourceSlot].occupied;
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

bool AssetResourceManager::FillReceipt(const Slot& slot, AssetResourceHandle handle, AssetResourceReceipt& receipt) const {
    if (!slot.occupied) return false;
    receipt = {slot.assetId, handle, slot.state, slot.contentHash, slot.refCount, slot.dependencyCount, slot.hotReloadGeneration, slot.generation};
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
        if ((slot.state == AssetResourceState::Stale || slot.contentHash != definition->contentHash) && slot.generation == std::numeric_limits<uint32_t>::max()) return Fail(AssetResourceError::Capacity);
    }
    if (totalLeaseCount_ > std::numeric_limits<uint32_t>::max() - closureCount || activeLeaseCount_ == std::numeric_limits<uint32_t>::max()) return Fail(AssetResourceError::RefcountOverflow);

    std::array<bool, kMaxResources> reserved{};
    for (uint16_t index = 0U; index < closureCount; ++index) if (targetSlots[index] == 0xFFFFU) {
        for (uint16_t candidate = 0U; candidate < kMaxResources; ++candidate) if (!slots_[candidate].occupied && !reserved[candidate]) { targetSlots[index] = candidate; reserved[candidate] = true; break; }
        if (targetSlots[index] == 0xFFFFU) return Fail(AssetResourceError::Capacity);
    }
    uint16_t leaseIndex = 0xFFFFU;
    for (uint16_t candidate = 0U; candidate < kMaxLeases; ++candidate) if (!leases_[candidate].occupied) { leaseIndex = candidate; break; }
    if (leaseIndex == 0xFFFFU) return Fail(AssetResourceError::Capacity);

    for (uint16_t index = 0U; index < closureCount; ++index) {
        Slot& slot = slots_[targetSlots[index]];
        const AssetDefinition* definition = registry_.Find(closureIds[index]);
        if (definition == nullptr) return Fail(AssetResourceError::MissingDependency);
        if (!slot.occupied) {
            const uint32_t generation = slot.generation;
            slot = {};
            slot.occupied = true;
            slot.assetId = closureIds[index];
            slot.generation = generation == 0U ? 1U : generation;
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
    for (uint16_t index = 0U; index < closureCount; ++index) ++slots_[targetSlots[index]].refCount;

    LeaseSlot& lease = leases_[leaseIndex];
    const uint32_t leaseGeneration = lease.generation == 0U ? 1U : lease.generation;
    lease = {};
    lease.occupied = true;
    lease.generation = leaseGeneration;
    lease.rootResourceSlot = targetSlots[0];
    lease.dependencyCount = static_cast<uint16_t>(closureCount - 1U);
    lease.dependencySlots.fill(0xFFFFU);
    for (uint16_t dependency = 1U; dependency < closureCount; ++dependency) lease.dependencySlots[dependency - 1U] = targetSlots[dependency];
    totalLeaseCount_ += closureCount;
    ++activeLeaseCount_;
    handle = {leaseIndex, lease.generation};
    lastError_ = AssetResourceError::None;
    return true;
}

bool AssetResourceManager::Release(AssetResourceHandle handle) {
    if (!ValidHandle(handle)) return Fail(AssetResourceError::InvalidHandle);
    LeaseSlot& lease = leases_[handle.slot];
    if (lease.generation == std::numeric_limits<uint32_t>::max()) return Fail(AssetResourceError::Capacity);
    const uint16_t targetCount = static_cast<uint16_t>(lease.dependencyCount + 1U);
    if (totalLeaseCount_ < targetCount || activeLeaseCount_ == 0U) return Fail(AssetResourceError::RefcountUnderflow);
    std::array<uint16_t, kMaxDependencyClosure> targets{};
    targets[0] = lease.rootResourceSlot;
    for (uint16_t index = 1U; index < targetCount; ++index) targets[index] = lease.dependencySlots[index - 1U];
    for (uint16_t index = 0U; index < targetCount; ++index) if (targets[index] >= kMaxResources || !slots_[targets[index]].occupied || slots_[targets[index]].refCount == 0U) return Fail(AssetResourceError::RefcountUnderflow);
    for (uint16_t index = 0U; index < targetCount; ++index) --slots_[targets[index]].refCount;
    totalLeaseCount_ -= targetCount;
    --activeLeaseCount_;
    lease.occupied = false;
    lease.rootResourceSlot = 0xFFFFU;
    lease.dependencyCount = 0U;
    lease.dependencySlots.fill(0xFFFFU);
    ++lease.generation;
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
        if (slots_[targetSlots[index]].generation == std::numeric_limits<uint32_t>::max()) return Fail(AssetResourceError::Capacity);
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
    const uint16_t rootSlot = FindSlot(assetId);
    if (rootSlot == 0xFFFFU) { lastError_ = AssetResourceError::None; return true; }
    std::array<bool, kMaxResources> affected{};
    affected[rootSlot] = true;
    for (uint16_t pass = 0U; pass < kMaxResources; ++pass) {
        bool changed = false;
        for (uint16_t candidate = 0U; candidate < kMaxResources; ++candidate) {
            const Slot& dependent = slots_[candidate];
            if (!dependent.occupied || affected[candidate]) continue;
            for (uint16_t dependency = 0U; dependency < dependent.dependencyCount; ++dependency) if (dependent.dependencySlots[dependency] < kMaxResources && affected[dependent.dependencySlots[dependency]]) {
                affected[candidate] = true;
                changed = true;
                break;
            }
        }
        if (!changed) break;
    }
    for (uint16_t index = 0U; index < kMaxResources; ++index) if (affected[index]) {
        if (slots_[index].refCount != 0U) return Fail(AssetResourceError::StaleInUse);
        if (slots_[index].generation == std::numeric_limits<uint32_t>::max()) return Fail(AssetResourceError::Capacity);
        const AssetDefinition* current = registry_.Find(slots_[index].assetId);
        if (current == nullptr || current->state != AssetState::Ready) return Fail(AssetResourceError::NotReady);
    }
    for (uint16_t index = 0U; index < kMaxResources; ++index) if (affected[index]) slots_[index].state = AssetResourceState::Stale;
    for (uint16_t index = 0U; index < kMaxResources; ++index) if (affected[index]) {
        const AssetDefinition* current = registry_.Find(slots_[index].assetId);
        if (!RefreshUnleasedSlot(slots_[index], *current)) return Fail(AssetResourceError::HotReloadRejected);
    }
    lastError_ = AssetResourceError::None;
    return true;
}

bool AssetResourceManager::EvictUnleased(uint16_t& evictedResources) {
    evictedResources = 0U;
    for (const Slot& slot : slots_) if (slot.occupied && slot.refCount == 0U && slot.generation == std::numeric_limits<uint32_t>::max()) return Fail(AssetResourceError::Capacity);
    for (Slot& slot : slots_) if (slot.occupied && slot.refCount == 0U) {
        const uint32_t nextGeneration = slot.generation + 1U;
        slot = {};
        slot.generation = nextGeneration == 0U ? 1U : nextGeneration;
        --activeResourceCount_;
        ++evictedResources;
    }
    lastError_ = AssetResourceError::None;
    return true;
}

bool AssetResourceManager::Query(AssetResourceHandle handle, AssetResourceReceipt& receipt) const {
    if (!ValidHandle(handle)) return Fail(AssetResourceError::InvalidHandle);
    const LeaseSlot& lease = leases_[handle.slot];
    return FillReceipt(slots_[lease.rootResourceSlot], handle, receipt);
}

bool AssetResourceManager::Query(std::string_view assetId, AssetResourceReceipt& receipt) const {
    const uint16_t slot = FindSlot(assetId);
    if (slot == 0xFFFFU) return Fail(AssetResourceError::InvalidIdentifier);
    return FillReceipt(slots_[slot], {}, receipt);
}

const std::vector<uint8_t>* AssetResourceManager::Data(AssetResourceHandle handle) const {
    if (!ValidHandle(handle)) return nullptr;
    const LeaseSlot& lease = leases_[handle.slot];
    const Slot& resource = slots_[lease.rootResourceSlot];
    if (resource.state != AssetResourceState::Ready) return nullptr;
    return registry_.Data(resource.assetId);
}

} // namespace NeoEngine
