#include "Demos/FarmInteractiveSurfaceDemo.h"

#include <cstdio>
#include <fstream>

int main() {
    using namespace NeoEngine;
    const std::string output = "farm_interactive_surface_demo_smoke.ppm";
    FarmInteractiveSurfaceDemoReceipt receipt{}; FarmInteractiveSurfaceDemoError error{};
    if (!RunFarmInteractiveSurfaceDemo({128U,96U,true,output},receipt,error) || error != FarmInteractiveSurfaceDemoError::None || receipt.frames != 19U || receipt.presentedFrames != 19U || receipt.worldFramebufferHash == 0U || receipt.hudFramebufferHash == receipt.worldFramebufferHash || receipt.selectedAction != FarmPlayerAction::Harvest || receipt.selectedActionMask != 0xFU || receipt.characterX != 2U || receipt.characterZ != 1U || receipt.telemetry.growingTiles != 0U || receipt.telemetry.harvestableTiles != 0U || receipt.telemetry.questHarvestProgress != 1U || receipt.inventory.wheatSeeds != 31U || receipt.inventory.wheatProduce != 2U) return 1;
    std::ifstream artifact(output,std::ios::binary); char magic[2]{}; artifact.read(magic,2); artifact.close(); std::remove(output.c_str());
    const FarmInteractiveSurfaceDemoReceipt preserved = receipt; FarmInteractiveSurfaceDemoReceipt rejected = receipt; FarmInteractiveSurfaceDemoError rejectedError{};
    if (RunFarmInteractiveSurfaceDemo({64U,96U,true,"invalid.ppm"},rejected,rejectedError) || rejectedError != FarmInteractiveSurfaceDemoError::InvalidConfiguration || rejected.frames != preserved.frames || rejected.hudFramebufferHash != preserved.hudFramebufferHash || magic[0] != 'P' || magic[1] != '6') return 1;
    std::printf("FARM_INTERACTIVE_SURFACE_DEMO_SMOKE_OK frames=%llu presented=%llu move=right actions=till,plant,water,harvest ppm=1 world=%llu hud=%llu\n",static_cast<unsigned long long>(receipt.frames),static_cast<unsigned long long>(receipt.presentedFrames),static_cast<unsigned long long>(receipt.worldFramebufferHash),static_cast<unsigned long long>(receipt.hudFramebufferHash));
    return 0;
}
