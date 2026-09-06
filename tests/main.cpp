// EOF
#include "NeoEngine/ActorComponentWorld.hpp"
#include "NeoEngine/AssetRefreshExecutor.hpp"
#include "NeoEngine/AuthoringCatalog.hpp"
#include <iostream>

int main() {
    NeoEngine::ActorComponentWorld world;
    NeoEngine::AssetRefreshExecutor executor;
    NeoEngine::AuthoringCatalog catalog;

    // 1. Test ECS Staging Validation
    const bool isCapacityValid = world.ValidateStagingCapacity(100);

    // 2. Test Asset Refresh Executor Routine
    NeoEngine::AssetRefreshEntry refreshEntry{NeoEngine::RefreshAction::RefreshTexture};
    executor.ExecuteRefresh(refreshEntry);

    // 3. Test Authoring Catalog Bone Validation
    NeoEngine::BoneData boneData{500U};
    const bool isBoneValid = catalog.ValidateBone(boneData);

    if (isCapacityValid && isBoneValid) {
        std::cout << "[NeoEngine Smoke Test] ALL CORE MODULES INTEGRITY PASSED." << std::endl;
        return 0;
    }

    std::cerr << "[NeoEngine Smoke Test] INTEGRITY VALIDATION FAILED." << std::endl;
    return 1;
}
