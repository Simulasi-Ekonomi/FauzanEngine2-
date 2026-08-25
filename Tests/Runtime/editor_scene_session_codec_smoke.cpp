#include "Runtime/EditorSceneSession.h"
#include "Runtime/SoftwareRenderer.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    const std::vector<uint8_t> ppm{'P','6','\n','1',' ','1','\n','2','5','5','\n',90U,230U,140U};
    AssetRegistry assets;
    if (!assets.ImportBytes("session-codec.sprite", AssetKind::Texture, {}, ppm) || !assets.MarkReady("session-codec.sprite")) return 1;
    const EditorSceneDocument document{EditorSceneDocument::kVersion, "session-codec", 9U, {{1U,0U,EditorSceneActorKind::Empty,{}}, {2U,1U,EditorSceneActorKind::Sprite,{0,0,3,0,0,0,1,1,1},"session-codec.sprite","","","",1,1,0,0,0xFFFFFFFFU}}};
    EditorSceneSession source;
    if (!source.Open(document, assets)) return 1;
    std::vector<uint8_t> bytes;
    if (!source.SaveBytes(bytes) || bytes.empty() || source.LastError() != EditorSceneSessionError::None) return 1;
    EditorSceneSession restored;
    if (!restored.OpenBytes(bytes, assets) || restored.LastError() != EditorSceneSessionError::None) return 1;
    EditorSceneDocument saved{};
    if (!restored.Save(saved) || saved.sceneId != document.sceneId || saved.revision != document.revision || saved.actors.size() != document.actors.size() || saved.actors[1].assetId != "session-codec.sprite") return 1;
    RenderCamera camera; SoftwareRenderer renderer;
    if (!camera.Initialize({RenderCameraMode::Perspective,{},5,90,1,0.1F,20}) || !renderer.Initialize(64,64) || !renderer.Clear(0xFF000000U) || !restored.RenderViewport(camera, renderer, {{0,0,-1}})) return 1;
    const uint64_t frameHash = renderer.FrameHash();
    std::vector<uint8_t> badMagic = bytes; badMagic[0] = 'X';
    if (restored.OpenBytes(badMagic, assets) || restored.LastError() != EditorSceneSessionError::CodecDecodeFailed || !restored.Save(saved) || saved.revision != document.revision || !renderer.Clear(0xFF000000U) || !restored.RenderViewport(camera, renderer, {{0,0,-1}}) || renderer.FrameHash() != frameHash) return 1;
    EditorSceneDocument missingAsset = document; missingAsset.revision = 10U; missingAsset.actors[1].assetId = "missing.sprite";
    EditorSceneDocumentCodec codec; std::vector<uint8_t> missingAssetBytes;
    if (!codec.Encode(missingAsset, missingAssetBytes) || restored.OpenBytes(missingAssetBytes, assets) || restored.LastError() != EditorSceneSessionError::DocumentLoadFailed || !restored.Save(saved) || saved.revision != document.revision || !renderer.Clear(0xFF000000U) || !restored.RenderViewport(camera, renderer, {{0,0,-1}}) || renderer.FrameHash() != frameHash) return 1;
    EditorSceneSession empty; std::vector<uint8_t> preservedBytes{9U};
    if (empty.SaveBytes(preservedBytes) || empty.LastError() != EditorSceneSessionError::InvalidDocument || preservedBytes != std::vector<uint8_t>{9U}) return 1;
    std::printf("EDITOR_SCENE_SESSION_CODEC_SMOKE_OK bytes=%zu roundTrip=1 malformedAtomic=1 assetOpenAtomic=1 hash=%llu\n", bytes.size(), static_cast<unsigned long long>(frameHash));
    return 0;
}
