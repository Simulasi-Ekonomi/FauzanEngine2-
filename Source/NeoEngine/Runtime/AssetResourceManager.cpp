#include "AssetResourceManager.h"

#include <algorithm>
#include <limits>
#include <new>

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
    return lease.occupied && lease.generation < std::numeric_limits<uint32_t>::max() - 1U && lease.generation == handle.generation && lease.rootResourceSlot < kMaxResources && slots_[lease.rootResourceSlot].occupied;
}

bool AssetResourceManager::BuildDependencyClosure(std::string_view assetId, std::array<std::string, kMaxDependencyClosure>& ids, uint16_t& count, std::array<std::string_view, kMaxDependencyDepth>& path, uint8_t depth, AssetResourceError& error) const {
    if (assetId.empty()) { error = AssetResourceError::MissingDependency; return false; }
    if (depth >= kMaxDependencyDepth) { error = AssetResourceError::Capacity; return false; }
    for (uint8_t index = 0U; index < depth; ++index) if (path[index] == assetId) { error = AssetResourceError::DependencyCycle; return false; }
    const AssetDefinition* definition = registry_.Find(assetId);
    if (definition == nullptr) { error = AssetResourceError::MissingDependency; return false; }
    if (definition->state != AssetState::Ready) { error = AssetResourceError::NotReady; return false; }
    for (uint16_t index = 0U; index < count; ++index) if (ids[index] == assetId) return true;
    if (count >= kMaxDependencyClosure) { error = AssetResourceError::Capacity; return false; }
    try {
        ids[count++] = std::string(assetId);
        path[depth] = assetId;
        for (const std::string& dependency : definition->dependencies) if (!BuildDependencyClosure(dependency, ids, count, path, static_cast<uint8_t>(depth + 1U), error)) return false;
        return true;
    } catch (const std::bad_alloc&) {
        error = AssetResourceError::Capacity;
        return false;
    }
}

bool AssetResourceManager::RefreshUnleasedSlot(Slot& slot, const AssetDefinition& definition) {
    if (slot.refCount != 0U) return Fail(AssetResourceError::StaleInUse);
    if (slot.generation >= std::numeric_limits<uint32_t>::max() - 2U || slot.hotReloadGeneration == std::numeric_limits<uint64_t>::max()) return Fail(AssetResourceError::Capacity);
    slot.state = AssetResourceState::Ready;
    slot.contentHash = definition.contentHash;
    ++slot.generation;
    ++slot.hotReloadGeneration;
    return true;
}

bool AssetResourceManager::FillReceipt(const Slot& slot, AssetResourceHandle handle, AssetResourceReceipt& receipt) const {
    if (!slot.occupied) return false;
    try {
        AssetResourceReceipt candidate{slot.assetId, handle, slot.state, slot.contentHash, slot.refCount, slot.dependencyCount, slot.hotReloadGeneration, slot.generation};
        receipt = std::move(candidate);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}

bool AssetResourceManager::Acquire(std::string_view assetId, AssetResourceHandle& handle) {
    if (!AssetRegistry::IsValidIdentifier(assetId)) return Fail(AssetResourceError::InvalidIdentifier);
    std::array<std::string, kMaxDependencyClosure> closureIds{};
    std::array<std::string_view, kMaxDependencyDepth> path{};
    uint16_t closureCount = 0U;
    AssetResourceError closureError = AssetResourceError::MissingDependency;
    if (!BuildDependencyClosure(assetId, closureIds, closureCount, path, 0U, closureError)) return Fail(closureError);

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
        if ((slot.state == AssetResourceState::Stale || slot.contentHash != definition->contentHash) && (slot.generation >= std::numeric_limits<uint32_t>::max() - 2U || slot.hotReloadGeneration == std::numeric_limits<uint64_t>::max())) return Fail(AssetResourceError::Capacity);
    }
    if (totalLeaseCount_ > std::numeric_limits<uint32_t>::max() - closureCount || activeLeaseCount_ == std::numeric_limits<uint32_t>::max()) return Fail(AssetResourceError::RefcountOverflow);

    std::array<bool, kMaxResources> reserved{};
    for (uint16_t index = 0U; index < closureCount; ++index) if (targetSlots[index] == 0xFFFFU) {
        for (uint16_t candidate = 0U; candidate < kMaxResources; ++candidate) if (!slots_[candidate].occupied && slots_[candidate].generation < std::numeric_limits<uint32_t>::max() - 2U && !reserved[candidate]) { targetSlots[index] = candidate; reserved[candidate] = true; break; }
        if (targetSlots[index] == 0xFFFFU) return Fail(AssetResourceError::Capacity);
    }
    uint16_t leaseIndex = 0xFFFFU;
    for (uint16_t candidate = 0U; candidate < kMaxLeases; ++candidate) if (!leases_[candidate].occupied && leases_[candidate].generation < std::numeric_limits<uint32_t>::max() - 1U) { leaseIndex = candidate; break; }
    if (leaseIndex == 0xFFFFU) return Fail(AssetResourceError::Capacity);
    if (managerRevision_ == std::numeric_limits<uint64_t>::max()) return Fail(AssetResourceError::Capacity);

    for (uint16_t index = 0U; index < closureCount; ++index) {
        Slot& slot = slots_[targetSlots[index]];
        const AssetDefinition* definition = registry_.Find(closureIds[index]);
        if (definition == nullptr) return Fail(AssetResourceError::MissingDependency);
        if (!slot.occupied) {
            const uint32_t generation = slot.generation;
            slot = {};
            slot.occupied = true;
            slot.assetId = std::move(closureIds[index]);
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
    ++managerRevision_;
    lastError_ = AssetResourceError::None;
    return true;
}

bool AssetResourceManager::Release(AssetResourceHandle handle) {
    if (!ValidHandle(handle)) return Fail(AssetResourceError::InvalidHandle);
    LeaseSlot& lease = leases_[handle.slot];
    if (lease.generation >= std::numeric_limits<uint32_t>::max() - 1U) return Fail(AssetResourceError::Capacity);
    if (managerRevision_ == std::numeric_limits<uint64_t>::max()) return Fail(AssetResourceError::Capacity);
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
    ++managerRevision_;
    lastError_ = AssetResourceError::None;
    return true;
}

bool AssetResourceManager::ReloadIfSafe(std::string_view assetId) {
    if (!AssetRegistry::IsValidIdentifier(assetId)) return Fail(AssetResourceError::InvalidIdentifier);
    const uint16_t rootSlot = FindSlot(assetId);
    if (rootSlot == 0xFFFFU) {
        const AssetDefinition* definition = registry_.Find(assetId);
        if (definition != nullptr && definition->state != AssetState::Ready) return Fail(AssetResourceError::NotReady);
        lastError_ = AssetResourceError::None;
        return true;
    }
    if (slots_[rootSlot].refCount != 0U) return Fail(AssetResourceError::StaleInUse);
    std::array<std::string, kMaxDependencyClosure> closureIds{};
    std::array<std::string_view, kMaxDependencyDepth> path{};
    uint16_t closureCount = 0U;
    AssetResourceError closureError = AssetResourceError::MissingDependency;
    if (!BuildDependencyClosure(assetId, closureIds, closureCount, path, 0U, closureError)) return Fail(closureError);
    std::array<uint16_t, kMaxDependencyClosure> targetSlots{};
    for (uint16_t index = 0U; index < closureCount; ++index) {
        targetSlots[index] = FindSlot(closureIds[index]);
        if (targetSlots[index] == 0xFFFFU || slots_[targetSlots[index]].refCount != 0U) return Fail(AssetResourceError::StaleInUse);
        if (slots_[targetSlots[index]].generation >= std::numeric_limits<uint32_t>::max() - 2U || slots_[targetSlots[index]].hotReloadGeneration == std::numeric_limits<uint64_t>::max()) return Fail(AssetResourceError::Capacity);
    }
    if (managerRevision_ == std::numeric_limits<uint64_t>::max()) return Fail(AssetResourceError::Capacity);
    for (uint16_t index = 0U; index < closureCount; ++index) {
        const AssetDefinition* definition = registry_.Find(closureIds[index]);
        if (definition == nullptr || definition->state != AssetState::Ready || !RefreshUnleasedSlot(slots_[targetSlots[index]], *definition)) return Fail(AssetResourceError::HotReloadRejected);
    }
    ++managerRevision_;
    lastError_ = AssetResourceError::None;
    return true;
}

bool AssetResourceManager::SyncHotReload(std::string_view assetId) {
    if (!AssetRegistry::IsValidIdentifier(assetId)) return Fail(AssetResourceError::InvalidIdentifier);
    const AssetDefinition* definition = registry_.Find(assetId);
    if (definition == nullptr) return Fail(AssetResourceError::MissingAsset);
    if (definition->state != AssetState::Ready) return Fail(AssetResourceError::NotReady);
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
        if (slots_[index].generation >= std::numeric_limits<uint32_t>::max() - 2U || slots_[index].hotReloadGeneration == std::numeric_limits<uint64_t>::max()) return Fail(AssetResourceError::Capacity);
        const AssetDefinition* current = registry_.Find(slots_[index].assetId);
        if (current == nullptr || current->state != AssetState::Ready) return Fail(AssetResourceError::NotReady);
    }
    if (managerRevision_ == std::numeric_limits<uint64_t>::max()) return Fail(AssetResourceError::Capacity);
    for (uint16_t index = 0U; index < kMaxResources; ++index) if (affected[index]) slots_[index].state = AssetResourceState::Stale;
    for (uint16_t index = 0U; index < kMaxResources; ++index) if (affected[index]) {
        const AssetDefinition* current = registry_.Find(slots_[index].assetId);
        if (!RefreshUnleasedSlot(slots_[index], *current)) return Fail(AssetResourceError::HotReloadRejected);
    }
    ++managerRevision_;
    lastError_ = AssetResourceError::None;
    return true;
}

uint32_t AssetResourceManager::ResidentBytes() const {
    uint64_t total = 0U;
    for (const Slot& slot : slots_) if (slot.occupied) {
        const AssetDefinition* definition = registry_.Find(slot.assetId);
        if (definition != nullptr) total += definition->byteSize;
    }
    return total > std::numeric_limits<uint32_t>::max() ? std::numeric_limits<uint32_t>::max() : static_cast<uint32_t>(total);
}

bool AssetResourceManager::PlanEviction(uint32_t maxResidentBytes, AssetEvictionPlan& plan) const {
    uint64_t total = 0U;
    uint64_t reclaimable = 0U;
    for (const Slot& slot : slots_) if (slot.occupied) {
        const AssetDefinition* definition = registry_.Find(slot.assetId);
        if (definition == nullptr) return Fail(AssetResourceError::HotReloadRejected);
        total += definition->byteSize;
        if (slot.refCount == 0U && slot.generation < std::numeric_limits<uint32_t>::max() - 2U) reclaimable += definition->byteSize;
    }
    if (total > std::numeric_limits<uint32_t>::max() || (total > maxResidentBytes && total - reclaimable > maxResidentBytes)) return Fail(AssetResourceError::BudgetExceeded);
    AssetEvictionPlan candidate{};
    candidate.managerRevision = managerRevision_;
    candidate.maxResidentBytes = maxResidentBytes;
    candidate.residentBytesBefore = static_cast<uint32_t>(total);
    candidate.activeResourcesBefore = activeResourceCount_;
    uint64_t candidateTotal = total;
    for (uint16_t index = 0U; index < kMaxResources && candidateTotal > maxResidentBytes; ++index) {
        const Slot& slot = slots_[index];
        if (!slot.occupied || slot.refCount != 0U || slot.generation >= std::numeric_limits<uint32_t>::max() - 2U) continue;
        const AssetDefinition* definition = registry_.Find(slot.assetId);
        if (definition == nullptr || definition->byteSize > candidateTotal || candidate.victimCount >= candidate.victims.size()) return Fail(AssetResourceError::HotReloadRejected);
        candidate.victims[candidate.victimCount++] = {index, slot.generation, definition->byteSize};
        candidateTotal -= definition->byteSize;
    }
    if (candidateTotal > maxResidentBytes) return Fail(AssetResourceError::BudgetExceeded);
    candidate.residentBytesAfter = static_cast<uint32_t>(candidateTotal);
    plan = candidate;
    lastError_ = AssetResourceError::None;
    return true;
}

bool AssetResourceManager::CommitEviction(const AssetEvictionPlan& plan) {
    if (plan.managerRevision != managerRevision_) return Fail(AssetResourceError::StaleEvictionPlan);
    AssetEvictionPlan expected{};
    if (!PlanEviction(plan.maxResidentBytes, expected)) return false;
    if (expected != plan) return Fail(AssetResourceError::InvalidEvictionPlan);
    if (plan.victimCount != 0U && managerRevision_ == std::numeric_limits<uint64_t>::max()) return Fail(AssetResourceError::Capacity);
    for (uint16_t index = 0U; index < plan.victimCount; ++index) {
        const AssetEvictionTarget& target = plan.victims[index];
        if (target.slot >= kMaxResources || !slots_[target.slot].occupied || slots_[target.slot].generation != target.generation || slots_[target.slot].refCount != 0U) return Fail(AssetResourceError::InvalidEvictionPlan);
        Slot& slot = slots_[target.slot];
        const uint32_t nextGeneration = slot.generation + 1U;
        slot = {};
        slot.generation = nextGeneration == 0U ? 1U : nextGeneration;
        --activeResourceCount_;
    }
    if (plan.victimCount != 0U) ++managerRevision_;
    lastError_ = AssetResourceError::None;
    return true;
}

bool AssetResourceManager::EvictToBudget(uint32_t maxResidentBytes, uint32_t& residentBytes, uint16_t& evictedResources) {
    AssetEvictionPlan plan{};
    if (!PlanEviction(maxResidentBytes, plan) || !CommitEviction(plan)) return false;
    residentBytes = plan.residentBytesAfter;
    evictedResources = plan.victimCount;
    return true;
}

bool AssetResourceManager::EvictUnleased(uint16_t& evictedResources) {
    for (const Slot& slot : slots_) if (slot.occupied && slot.refCount == 0U && slot.generation >= std::numeric_limits<uint32_t>::max() - 2U) return Fail(AssetResourceError::Capacity);
    bool willEvict = false;
    for (const Slot& slot : slots_) if (slot.occupied && slot.refCount == 0U) { willEvict = true; break; }
    if (willEvict && managerRevision_ == std::numeric_limits<uint64_t>::max()) return Fail(AssetResourceError::Capacity);
    uint16_t candidateEvicted = 0U;
    for (Slot& slot : slots_) if (slot.occupied && slot.refCount == 0U && slot.generation < std::numeric_limits<uint32_t>::max() - 2U) {
        const uint32_t nextGeneration = slot.generation + 1U;
        slot = {};
        slot.generation = nextGeneration == 0U ? 1U : nextGeneration;
        --activeResourceCount_;
        ++candidateEvicted;
    }
    evictedResources = candidateEvicted;
    if (candidateEvicted != 0U) ++managerRevision_;
    lastError_ = AssetResourceError::None;
    return true;
}

bool AssetResourceManager::Query(AssetResourceHandle handle, AssetResourceReceipt& receipt) const {
    if (!ValidHandle(handle)) return Fail(AssetResourceError::InvalidHandle);
    const LeaseSlot& lease = leases_[handle.slot];
    if (!FillReceipt(slots_[lease.rootResourceSlot], handle, receipt)) return Fail(AssetResourceError::Capacity);
    lastError_ = AssetResourceError::None;
    return true;
}

bool AssetResourceManager::Query(std::string_view assetId, AssetResourceReceipt& receipt) const {
    if (!AssetRegistry::IsValidIdentifier(assetId)) return Fail(AssetResourceError::InvalidIdentifier);
    const uint16_t slot = FindSlot(assetId);
    if (slot == 0xFFFFU) return Fail(AssetResourceError::MissingAsset);
    if (!FillReceipt(slots_[slot], {}, receipt)) return Fail(AssetResourceError::Capacity);
    lastError_ = AssetResourceError::None;
    return true;
}

const std::vector<uint8_t>* AssetResourceManager::Data(AssetResourceHandle handle) const {
    if (!ValidHandle(handle)) return nullptr;
    const LeaseSlot& lease = leases_[handle.slot];
    const Slot& resource = slots_[lease.rootResourceSlot];
    const AssetDefinition* definition = registry_.Find(resource.assetId);
    if (resource.state != AssetResourceState::Ready || definition == nullptr || definition->state != AssetState::Ready) { lastError_ = definition == nullptr ? AssetResourceError::MissingAsset : AssetResourceError::NotReady; return nullptr; }
    if (definition->contentHash != resource.contentHash) { lastError_ = AssetResourceError::StaleInUse; return nullptr; }
    const std::vector<uint8_t>* data = registry_.Data(resource.assetId);
    if (data == nullptr) { lastError_ = AssetResourceError::MissingAsset; return nullptr; }
    lastError_ = AssetResourceError::None;
    return data;
}

} // namespace NeoEngine
