#pragma once

#include "AssetRefreshDiagnostics.h"
#include "SceneSpriteAdapter.h"

#include <cstdint>
#include <string>
#include <vector>

namespace NeoEngine {

enum class AssetRefreshExecutorError : uint8_t { None, Capacity, PlanInvalid, PlanStale, MissingInstance, MissingResource, StaleResource, ProbeFailed, ActionFailed };

struct AssetRefreshReceipt {
    AssetRefreshAction action = AssetRefreshAction::RefreshTexture;
    std::string assetId;
    std::string materialName;
    SceneEntity entity{};
    bool succeeded = false;
};

struct AssetRefreshPreflightReceipt {
    AssetRefreshAction action = AssetRefreshAction::RefreshTexture;
    std::string assetId;
    std::string materialName;
    SceneEntity entity{};
    bool structurallyValid = false;
};

class AssetRefreshExecutor {
public:
    static constexpr size_t kMaxReceipts = AssetRefreshDiagnostics::kMaxEntries;

    // Structural validation only; it does not decode/import candidate bytes or mutate any resource.
    bool Preflight(const std::vector<AssetRefreshPlanEntry>& plan, const AssetRegistry& registry, const TextureStagingStore& textures, const MeshStagingStore& meshes, const MaterialStagingStore& materials, const SceneMeshAdapter& scene);
    bool Execute(const std::vector<AssetRefreshPlanEntry>& plan, const AssetRegistry& registry, TextureStagingStore& textures, MeshStagingStore& meshes, MaterialStagingStore& materials, SceneMeshAdapter& scene);
    // Runs the same bounded plan against candidate staging/scene copies and commits all only after success.
    bool ExecuteAtomic(const std::vector<AssetRefreshPlanEntry>& plan, const AssetRegistry& registry, TextureStagingStore& textures, MeshStagingStore& meshes, MaterialStagingStore& materials, SceneMeshAdapter& scene);
    // Bounded texture-plus-sprite plan. Diagnostics may supply multiple distinct
    // sprite-instance actions; both stores and receipts commit only on success.
    bool ExecuteSpritesAtomic(const std::vector<AssetRefreshPlanEntry>& plan, const AssetRegistry& registry, TextureStagingStore& textures, SceneSpriteAdapter& sprites);
    [[nodiscard]] const std::vector<AssetRefreshPreflightReceipt>& PreflightReceipts() const { return preflightReceipts_; }
    [[nodiscard]] const std::vector<AssetRefreshReceipt>& Receipts() const { return receipts_; }
    [[nodiscard]] AssetRefreshExecutorError LastError() const { return lastError_; }

private:
    std::vector<AssetRefreshPreflightReceipt> preflightReceipts_;
    std::vector<AssetRefreshReceipt> receipts_;
    AssetRefreshExecutorError lastError_ = AssetRefreshExecutorError::None;
};

} // namespace NeoEngine
