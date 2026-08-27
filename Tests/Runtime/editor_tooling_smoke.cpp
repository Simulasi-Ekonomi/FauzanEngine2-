#include "Runtime/EditorSceneSession.h"
#include "Runtime/SoftwareRenderer.h"

#include <cstdio>
#include <limits>
#include <vector>

int main() {
    using namespace NeoEngine;

    const std::vector<uint8_t> pixel{'P','6','\n','1',' ','1','\n','2','5','5','\n',220U,120U,40U};
    AssetRegistry assets;
    if (!assets.ImportBytes("editor_v1.sprite", AssetKind::Texture, {}, pixel) || !assets.MarkReady("editor_v1.sprite")) return 1;

    EditorSceneDocument document{
        EditorSceneDocument::kVersion,
        "editor-v1",
        1,
        {
            EditorSceneActor{1U, 0U, EditorSceneActorKind::Empty, {}, "", "", "", "", 1.0F, 1.0F, 0, 0, 0xFFFFFFFFU},
            EditorSceneActor{2U, 1U, EditorSceneActorKind::Sprite, {0.0F, 0.0F, 3.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}, "editor_v1.sprite", "", "", "", 1.0F, 1.0F, 1, 0, 0xFFFFFFFFU},
        },
    };

    EditorSceneSession session;
    if (!session.Open(document, assets) || session.LastError() != EditorSceneSessionError::None) return 1;
    const auto hierarchy = session.HierarchySnapshot();
    if (hierarchy.size() != 2U || hierarchy[0].id != 1U || hierarchy[1].id != 2U) return 1;

    EditorSceneActor inspected{};
    if (!session.SelectActor(2U) || !session.InspectSelected(inspected) || inspected.assetId != "editor_v1.sprite") return 1;

    RenderCamera camera;
    SoftwareRenderer renderer;
    if (!camera.Initialize({RenderCameraMode::Perspective, {}, 5.0F, 90.0F, 1.0F, 0.1F, 20.0F}) ||
        !renderer.Initialize(64U, 64U) || !renderer.Clear(0xFF101018U) ||
        !session.RenderViewport(camera, renderer, {{0.0F, 0.0F, -1.0F}}) || renderer.FrameHash() == 0U) return 1;
    const uint64_t originalHash = renderer.FrameHash();

    if (!session.UpdateTransform(2U, {0.5F, 0.0F, 3.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}, assets) || !session.HasUnsavedChanges()) return 1;
    if (!session.Undo(assets) || !session.InspectActor(2U, inspected) || inspected.transform.x != 0.0F) return 1;
    if (!session.Redo(assets) || !session.InspectActor(2U, inspected) || inspected.transform.x != 0.5F) return 1;

    EditorSceneDocument saved{};
    if (!session.Save(saved) || session.HasUnsavedChanges() || saved.revision != 2U) return 1;
    if (!renderer.Clear(0xFF101018U) || !session.RenderViewport(camera, renderer, {{0.0F, 0.0F, -1.0F}})) return 1;
    const uint64_t movedHash = renderer.FrameHash();
    if (movedHash == originalHash) return 1;
    std::vector<uint8_t> bytes;
    if (!session.SaveBytes(bytes) || bytes.empty()) return 1;

    if (session.UpdateTransform(2U, {std::numeric_limits<float>::quiet_NaN(), 0.0F, 3.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}, assets)) return 1;
    if (!renderer.Clear(0xFF101018U) || !session.RenderViewport(camera, renderer, {{0.0F, 0.0F, -1.0F}}) || renderer.FrameHash() != movedHash) return 1;

    EditorSceneSession restored;
    if (!restored.OpenBytes(bytes, assets) || !restored.SelectActor(2U) || !restored.InspectSelected(inspected) || inspected.transform.x != 0.5F) return 1;
    if (!restored.DeleteActor(2U, assets) || restored.HierarchySnapshot().size() != 1U) return 1;

    std::printf("EDITOR_TOOLING_SMOKE_OK hierarchy=1 selection=1 inspector=1 viewport=1 transform=1 undo=1 redo=1 save=1 load=1 rollback=1 delete=1 hash=%llu\n", static_cast<unsigned long long>(originalHash));
    return 0;
}

// Editor V1 acceptance: the same bounded scene session owns authoring, validation,
// viewport rendering, and failure-preserving history; no parallel mock path is used.
