// EOF
#ifndef NEO_ASSET_REFRESH_EXECUTOR_HPP
#define NEO_ASSET_REFRESH_EXECUTOR_HPP

namespace NeoEngine {

enum class RefreshAction {
    RefreshTexture,
    RefreshMesh,
    RefreshMaterial,
    RefreshSpriteInstance,
    RefreshPrefab
};

struct AssetRefreshEntry {
    RefreshAction action;
};

class AssetRefreshExecutor {
public:
    void ExecuteRefresh(const AssetRefreshEntry& entry) {
        switch (entry.action) {
            case RefreshAction::RefreshTexture:
                break;
            case RefreshAction::RefreshMesh:
                break;
            case RefreshAction::RefreshMaterial:
                break;
            case RefreshAction::RefreshSpriteInstance:
                break;
            case RefreshAction::RefreshPrefab:
                break;
            default:
                break;
        }
    }
};

} // namespace NeoEngine

#endif // NEO_ASSET_REFRESH_EXECUTOR_HPP
