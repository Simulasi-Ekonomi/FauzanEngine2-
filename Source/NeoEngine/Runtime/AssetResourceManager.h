#pragma once

#include "AssetRegistry.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace NeoEngine {

enum class AssetResourceState : uint8_t { Ready, Stale };
enum class AssetResourceError : uint8_t {
    None,
    InvalidIdentifier,
    NotReady,
    MissingDependency,
    DependencyCycle,
    Capacity,
    StaleInUse,
    InvalidHandle,
    RefcountOverflow,
    RefcountUnderflow,
    HotReloadRejected,
    BudgetExceeded,
    MissingAsset,
    InvalidEvictionPlan,
    StaleEvictionPlan,
};

struct AssetResourceHandle {
    uint16_t slot = 0xFFFFU;
    uint32_t generation = 0U;
    bool operator==(const AssetResourceHandle&) const = default;
};

struct AssetResourceReceipt {
    std::string assetId;
    AssetResourceHandle handle{};
    AssetResourceState state = AssetResourceState::Ready;
    uint64_t contentHash = 0U;
    uint32_t refCount = 0U;
    uint16_t dependencyCount = 0U;
    uint64_t hotReloadGeneration = 0U;
    uint32_t resourceGeneration = 0U;
};

struct AssetEvictionTarget {
    uint16_t slot = 0xFFFFU;
    uint32_t generation = 0U;
    uint32_t byteSize = 0U;
    bool operator==(const AssetEvictionTarget&) const = default;
};

struct AssetEvictionPlan {
    uint64_t managerRevision = 0U;
    uint32_t maxResidentBytes = 0U;
    uint32_t residentBytesBefore = 0U;
    uint32_t residentBytesAfter = 0U;
    uint16_t activeResourcesBefore = 0U;
    uint16_t victimCount = 0U;
    std::array<AssetEvictionTarget, AssetRegistry::kMaxAssets> victims{};
    bool operator==(const AssetEvictionPlan&) const = default;
};

class AssetResourceManager {
public:
    static constexpr uint16_t kMaxResources = static_cast<uint16_t>(AssetRegistry::kMaxAssets);
    static constexpr uint8_t kMaxDependencyDepth = 16U;
    static constexpr uint16_t kMaxDependencyClosure = 64U;
    static constexpr uint16_t kMaxLeases = 8192U;

    explicit AssetResourceManager(const AssetRegistry& registry);
    AssetResourceManager(const AssetResourceManager&) = delete;
    AssetResourceManager& operator=(const AssetResourceManager&) = delete;

    bool Acquire(std::string_view assetId, AssetResourceHandle& handle);
    bool Release(AssetResourceHandle handle);
    bool SyncHotReload(std::string_view assetId);
    bool ReloadIfSafe(std::string_view assetId);
    bool EvictUnleased(uint16_t& evictedResources);
    bool EvictToBudget(uint32_t maxResidentBytes, uint32_t& residentBytes, uint16_t& evictedResources);
    bool PlanEviction(uint32_t maxResidentBytes, AssetEvictionPlan& plan) const;
    bool CommitEviction(const AssetEvictionPlan& plan);
    bool Query(AssetResourceHandle handle, AssetResourceReceipt& receipt) const;
    bool Query(std::string_view assetId, AssetResourceReceipt& receipt) const;
    const std::vector<uint8_t>* Data(AssetResourceHandle handle) const;

    [[nodiscard]] uint16_t ActiveResourceCount() const { return activeResourceCount_; }
    [[nodiscard]] uint32_t ResidentBytes() const;
    [[nodiscard]] uint32_t TotalLeaseCount() const { return totalLeaseCount_; }
    [[nodiscard]] uint32_t ActiveLeaseCount() const { return activeLeaseCount_; }
    [[nodiscard]] AssetResourceError LastError() const { return lastError_; }

private:
    struct Slot {
        bool occupied = false;
        std::string assetId;
        uint32_t generation = 1U;
        AssetResourceState state = AssetResourceState::Ready;
        uint64_t contentHash = 0U;
        uint64_t hotReloadGeneration = 0U;
        uint32_t refCount = 0U;
        uint16_t dependencyCount = 0U;
        std::array<uint16_t, kMaxDependencyClosure> dependencySlots{};
    };

    struct LeaseSlot {
        bool occupied = false;
        uint32_t generation = 1U;
        uint16_t rootResourceSlot = 0xFFFFU;
        uint16_t dependencyCount = 0U;
        std::array<uint16_t, kMaxDependencyClosure> dependencySlots{};
    };

    bool Fail(AssetResourceError error) const;
    bool BuildDependencyClosure(std::string_view assetId, std::array<std::string, kMaxDependencyClosure>& ids, uint16_t& count, std::array<std::string_view, kMaxDependencyDepth>& path, uint8_t depth, AssetResourceError& error) const;
    uint16_t FindSlot(std::string_view assetId) const;
    bool ValidHandle(AssetResourceHandle handle) const;
    bool RefreshUnleasedSlot(Slot& slot, const AssetDefinition& definition);
    bool FillReceipt(const Slot& slot, AssetResourceHandle handle, AssetResourceReceipt& receipt) const;

    const AssetRegistry& registry_;
    std::array<Slot, kMaxResources> slots_{};
    std::array<LeaseSlot, kMaxLeases> leases_{};
    uint16_t activeResourceCount_ = 0U;
    uint32_t totalLeaseCount_ = 0U;
    uint32_t activeLeaseCount_ = 0U;
    uint64_t managerRevision_ = 1U;
    mutable AssetResourceError lastError_ = AssetResourceError::None;
};

} // namespace NeoEngine
