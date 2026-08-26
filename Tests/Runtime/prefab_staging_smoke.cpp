#include "Runtime/EditorScenePrefabCodec.h"
#include "Runtime/EditorSceneSession.h"
#include "Runtime/AssetRefreshExecutor.h"
#include "Runtime/PrefabStaging.h"

#include <cstdio>
#include <vector>

int main() {
    using namespace NeoEngine;
    const EditorScenePrefab source{20U, {
        {20U, 0U, EditorSceneActorKind::Empty, {2, 0, 0, 0, 0, 0, 1, 1, 1}},
        {21U, 20U, EditorSceneActorKind::Marker, {1, 0, 0, 0, 0, 0, 1, 1, 1}},
    }};
    EditorScenePrefabCodec codec;
    std::vector<uint8_t> prefabBytes;
    if (!codec.Encode(source, prefabBytes)) return 1;
    const EditorScenePrefab replacement{20U, {
        {20U, 0U, EditorSceneActorKind::Empty, {3, 0, 0, 0, 0, 0, 1, 1, 1}},
        {21U, 20U, EditorSceneActorKind::Marker, {1, 0, 0, 0, 0, 0, 1, 1, 1}},
    }};
    std::vector<uint8_t> replacementBytes;
    if (!codec.Encode(replacement, replacementBytes)) return 1;
    AssetRegistry assets;
    const std::vector<uint8_t> broken{'B', 'A', 'D'};
    if (!assets.ImportBytes("farm.house", AssetKind::Prefab, {}, prefabBytes) || !assets.MarkReady("farm.house") || !assets.ImportBytes("farm.broken", AssetKind::Prefab, {}, broken) || !assets.MarkReady("farm.broken")) return 1;
    PrefabStagingStore staging;
    if (!staging.Stage(assets, "farm.house") || !staging.IsCurrent(assets, "farm.house") || staging.ResourceCount() != 1U || staging.StagedActors() != 2U || staging.Stage(assets, "farm.broken") || staging.LastError() != PrefabStagingError::DecodeFailed || staging.ResourceCount() != 1U || staging.StagedActors() != 2U) return 1;
    TextureStagingStore textures; MeshStagingStore meshes; MaterialStagingStore materials; SceneMeshAdapter scene; AssetRefreshDiagnostics diagnostics; AssetRefreshExecutor executor;

    const EditorSceneDocument document{EditorSceneDocument::kVersion, "prefab-staging", 1U, {{10U, 0U, EditorSceneActorKind::Empty, {10, 0, 0, 0, 0, 0, 1, 1, 1}}}};
    EditorSceneSession session;
    if (!session.Open(document, assets) || !session.InstantiateStagedPrefab(staging, "farm.house", 10U, {100U, 101U}, assets)) return 1;
    EditorSceneDocument saved{};
    EditorSceneActor root{};
    EditorSceneActor child{};
    if (!session.Save(saved) || saved.revision != 2U || saved.actors.size() != 3U || !session.InspectActor(100U, root) || !session.InspectActor(101U, child) || root.parentId != 10U || child.parentId != 100U) return 1;
    const uint64_t preservedRevision = saved.revision;

    std::vector<uint8_t> changedBytes = prefabBytes;
    changedBytes[0] = 'X';
    if (!assets.ReplaceBytes("farm.house", changedBytes) || staging.IsCurrent(assets, "farm.house") || staging.CanRefresh(assets, "farm.house") || session.InstantiateStagedPrefab(staging, "farm.house", 10U, {102U, 103U}, assets) || session.LastError() != EditorSceneSessionError::InvalidDocument || !session.Save(saved) || saved.revision != preservedRevision || saved.actors.size() != 3U) return 1;
    if (!assets.ReplaceBytes("farm.house", replacementBytes) || !diagnostics.BuildPlan(assets, "farm.house", textures, meshes, materials, scene, staging) || diagnostics.Entries().size() != 1U || diagnostics.Entries()[0].action != AssetRefreshAction::RefreshPrefab || !executor.ExecutePrefabsAtomic(diagnostics.Entries(), assets, staging) || !staging.IsCurrent(assets, "farm.house") || executor.PreflightReceipts().size() != 1U || !executor.PreflightReceipts()[0].structurallyValid || executor.Receipts().size() != 1U || !executor.Receipts()[0].succeeded || !session.Save(saved) || saved.revision != preservedRevision || saved.actors.size() != 3U) return 1;
    if (!session.InstantiateStagedPrefab(staging, "farm.house", 10U, {102U, 103U}, assets) || !session.Save(saved) || saved.revision != 3U || saved.actors.size() != 5U) return 1;
    const uint64_t refreshedHash = staging.Find("farm.house")->sourceHash; const std::vector<AssetRefreshReceipt> retainedReceipts = executor.Receipts(); const std::vector<AssetRefreshPreflightReceipt> retainedPreflight = executor.PreflightReceipts();
    if (!assets.ReplaceBytes("farm.house", changedBytes) || !diagnostics.BuildPlan(assets, "farm.house", textures, meshes, materials, scene, staging) || diagnostics.Entries().size() != 1U || executor.ExecutePrefabsAtomic(diagnostics.Entries(), assets, staging) || executor.LastError() != AssetRefreshExecutorError::ProbeFailed || staging.Find("farm.house")->sourceHash != refreshedHash || staging.IsCurrent(assets, "farm.house") || executor.Receipts().size() != retainedReceipts.size() || executor.PreflightReceipts().size() != retainedPreflight.size() || session.InstantiateStagedPrefab(staging, "farm.house", 10U, {104U, 105U}, assets) || !session.Save(saved) || saved.revision != 3U || saved.actors.size() != 5U) return 1;
    std::printf("PREFAB_STAGING_SMOKE_OK ready=1 stale=1 diagnostics=1 atomic=1 sessionPreserved=1 receiptsPreserved=1 actors=%u\n", session.World().AliveCount());
    return 0;
}
