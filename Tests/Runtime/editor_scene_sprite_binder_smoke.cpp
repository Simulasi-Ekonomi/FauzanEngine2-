#include "Runtime/AssetRegistry.h"
#include "Runtime/EditorSceneDocument.h"
#include "Runtime/EditorSceneSpriteBinder.h"
#include "Runtime/RenderCamera.h"
#include "Runtime/SceneSpriteAdapter.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/SpriteBatch.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    const auto require = [](bool condition, const char* stage) { if (!condition) std::fprintf(stderr, "EDITOR_SCENE_SPRITE_BINDER_SMOKE_FAIL stage=%s\n", stage); return condition; };
    const std::vector<uint8_t> ppm{'P', '6', '\n', '1', ' ', '1', '\n', '2', '5', '5', '\n', static_cast<uint8_t>(0x28U), static_cast<uint8_t>(0xA0U), static_cast<uint8_t>(0xE0U)};
    AssetRegistry assets;
    if (!require(assets.ImportBytes("farmer.ppm", AssetKind::Texture, {}, ppm) && assets.MarkReady("farmer.ppm"), "asset-ready")) return 1;
    EditorSceneDocument document{};
    document.sceneId = "sprite-farm";
    document.revision = 1;
    document.actors.push_back({.id = 1, .kind = EditorSceneActorKind::Sprite, .transform = {0, 0, 1, 0, 0, 0, 1, 1, 1}, .assetId = "farmer.ppm", .spriteWidth = 4.0F, .spriteHeight = 4.0F, .spriteLayer = 2, .spriteOrder = 3});
    SceneWorld world;
    EditorSceneDocumentAdapter documentAdapter;
    if (!require(documentAdapter.Load(document, assets, world) && world.AliveCount() == 1U, "document-load")) return 1;
    TextureStagingStore textures;
    SceneSpriteAdapter sprites;
    EditorSceneSpriteBinder binder;
    if (!require(binder.BindDocumentAssets(document, documentAdapter, assets, textures, sprites) && sprites.InstanceCount() == 1U && textures.ResourceCount() == 1U, "sprite-bind")) return 1;
    SoftwareRenderer renderer;
    RenderCamera camera;
    SpriteBatch batch;
    if (!require(renderer.Initialize(64, 64) && renderer.Clear(0xFF000000U) && camera.Initialize({RenderCameraMode::Orthographic, {}, 5.0F, 60.0F, 1.0F, 0.1F, 10.0F}) && sprites.Queue(world, batch) && batch.Flush(renderer, camera) && renderer.PixelAt(32, 32) == 0xFF28A0E0U, "textured-draw")) return 1;
    const SceneEntity* entity = documentAdapter.EntityForActor(1);
    const std::vector<uint8_t> refreshedPpm{'P', '6', '\n', '1', ' ', '1', '\n', '2', '5', '5', '\n', static_cast<uint8_t>(0xE0U), static_cast<uint8_t>(0x78U), static_cast<uint8_t>(0x28U)};
    if (!require(entity != nullptr && assets.ReplaceBytes("farmer.ppm", refreshedPpm) && textures.Refresh(assets, "farmer.ppm") && sprites.RefreshStaged(*entity, *textures.Find("farmer.ppm")), "sprite-refresh")) return 1;
    batch.Clear();
    if (!require(renderer.Clear(0xFF000000U) && sprites.Queue(world, batch) && batch.Flush(renderer, camera) && renderer.PixelAt(32, 32) == 0xFFE07828U, "sprite-refresh-draw")) return 1;
    const uint64_t refreshedHash = renderer.FrameHash();
    CpuTextureResource malformedRefresh = *textures.Find("farmer.ppm");
    malformedRefresh.rgba.clear();
    if (!require(!sprites.RefreshStaged(*entity, malformedRefresh) && sprites.LastError() == SceneSpriteAdapterError::InvalidResource, "sprite-refresh-reject")) return 1;
    batch.Clear();
    if (!require(renderer.Clear(0xFF000000U) && sprites.Queue(world, batch) && batch.Flush(renderer, camera) && renderer.FrameHash() == refreshedHash, "sprite-refresh-preserve")) return 1;
    if (!require(entity != nullptr && world.SetTransform(*entity, {3, 0, 1, 0, 0, 0, 1, 1, 1}) && world.UpdateTransforms(), "world-move")) return 1;
    batch.Clear();
    const bool clearMoved = renderer.Clear(0xFF000000U);
    const bool queueMoved = sprites.Queue(world, batch);
    const bool flushMoved = batch.Flush(renderer, camera);
    const uint32_t movedCenter = renderer.PixelAt(32, 32);
    if (!require(clearMoved && queueMoved && flushMoved && movedCenter != 0xFF28A0E0U, "moved-draw")) { std::fprintf(stderr, "EDITOR_SCENE_SPRITE_BINDER_SMOKE_MOVED clear=%d queue=%d flush=%d pixel=%08X\n", clearMoved, queueMoved, flushMoved, movedCenter); return 1; }
    const uint64_t movedHash = renderer.FrameHash();
    EditorSceneDocument noSprite{.version = 3, .sceneId = "sprite-farm", .revision = 2, .actors = {{.id = 2, .kind = EditorSceneActorKind::Marker, .transform = {0, 0, 1, 0, 0, 0, 1, 1, 1}}}};
    if (!require(!binder.BindDocumentAssets(noSprite, documentAdapter, assets, textures, sprites) && binder.LastError() == EditorSceneSpriteBinderError::InvalidDocument && sprites.InstanceCount() == 1U, "atomic-rejection")) return 1;
    const std::vector<uint8_t> malformed{'N', 'O'};
    if (!require(assets.ImportBytes("broken.ppm", AssetKind::Texture, {}, malformed) && assets.MarkReady("broken.ppm"), "broken-ready")) return 1;
    EditorSceneDocument laterFailure = document;
    laterFailure.revision = 3;
    laterFailure.actors.push_back({.id = 2, .kind = EditorSceneActorKind::Sprite, .transform = {2, 0, 1, 0, 0, 0, 1, 1, 1}, .assetId = "broken.ppm", .spriteWidth = 2.0F, .spriteHeight = 2.0F});
    if (!require(documentAdapter.Load(laterFailure, assets, world), "later-document-load")) return 1;
    TextureStagingStore atomicTextures;
    SceneSpriteAdapter atomicSprites = sprites;
    if (!require(!binder.BindDocumentAssets(laterFailure, documentAdapter, assets, atomicTextures, atomicSprites) && binder.LastError() == EditorSceneSpriteBinderError::TextureStageFailed && atomicTextures.ResourceCount() == 0U && atomicSprites.InstanceCount() == 1U, "later-atomic-rollback")) return 1;
    std::printf("EDITOR_SCENE_SPRITE_BINDER_SMOKE_OK sprites=%zu texture=1 moved=1 hash=%llu\n", sprites.InstanceCount(), static_cast<unsigned long long>(movedHash));
    return 0;
}
