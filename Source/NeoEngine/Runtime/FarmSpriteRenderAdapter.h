#pragma once

#include <cstdint>
#include <string>

namespace NeoEngine {
class AssetRegistry;
class FarmSystem;
class FarmWorldTool;
class SoftwareRenderer;
class TextureStagingStore;

enum class FarmSpriteRenderError : uint8_t { None, NotReady, WorldMismatch, Capacity, InvalidAssetConfig, TextureStageFailed, RendererUnavailable, QueueRejected, RenderFailed };

struct FarmSpriteAssetSet {
    std::string emptyTile;
    std::string tilledTile;
    std::string growingTile;
    std::string harvestableTile;
    std::string farmhouse;
    std::string barn;
    std::string silo;
    std::string market;
    std::string workshop;
    std::string townHall;
    std::string farmer;
    std::string builder;
    std::string merchant;
    std::string questGiver;
    std::string ranger;
    std::string player;
};

// Read-only view over FarmSystem/FarmWorldTool. A completed call atomically
// replaces the output SoftwareRenderer frame, but does not mutate game state.
class FarmSpriteRenderAdapter {
public:
    bool RenderWorld(const FarmSystem& farm, const FarmWorldTool& world, const FarmSpriteAssetSet& assetSet, const AssetRegistry& assets, TextureStagingStore& textures, SoftwareRenderer& renderer);
    [[nodiscard]] FarmSpriteRenderError LastError() const { return lastError_; }
private:
    bool Fail(FarmSpriteRenderError error);
    FarmSpriteRenderError lastError_ = FarmSpriteRenderError::None;
};
} // namespace NeoEngine
