#include "Runtime/NeoRuntime.h"

#include <cstdio>
#include <vector>

int main() {
    using namespace NeoEngine;

    RuntimeConfig config{};
    config.farmWidth = 4U;
    config.farmHeight = 4U;
    config.renderWidth = 128U;
    config.renderHeight = 96U;
    config.farmNpcCount = 4U;
    config.enableFarmRuntimeHud = true;
    config.enableSoftwareSurfacePresentation = true;
    config.softwareSurfaceHidden = true;
    config.enableFarmPlayerInput = true;

    NeoRuntime runtime;
    int stage = 1;
    bool ok = runtime.Initialize(config) && runtime.Input() != nullptr && runtime.FarmWorld() != nullptr;
    const auto right = MakeInputCode(InputDeviceType::Keyboard, 23U);
    const auto interact = MakeInputCode(InputDeviceType::Keyboard, 24U);

    stage = 2;
    ok = ok && runtime.Input()->Push(right, true) && runtime.Tick();
    const NeoRuntimeFrameReceipt* movedReceipt = runtime.LastFrameReceipt();
    ok = ok && movedReceipt != nullptr && movedReceipt->hasFarmPlayerInputReceipt &&
         movedReceipt->farmPlayerInput.kind == FarmPlayerInputKind::Movement &&
         movedReceipt->farmPlayerInput.x == 1U && runtime.FarmWorld()->Character().x == 1U;

    FarmActionPanelReceipt panel{};
    stage = 3;
    ok = ok && runtime.Input()->Push(right, false) && runtime.Tick() && runtime.RenderFarm();
    const RuntimeFarmRenderReceipt* firstRender = runtime.LastFarmRenderReceipt();
    ok = ok && firstRender != nullptr && firstRender->frame == 1U && firstRender->presentedFrameCount == 1U &&
         firstRender->worldFramebufferHash != 0U && firstRender->hudFramebufferHash != 0U;

    stage = 4;
    ok = ok && runtime.RouteFarmHudKeyboard(UiKeyboardKey::TabForward, panel) &&
         runtime.RouteFarmHudKeyboard(UiKeyboardKey::Activate, panel) && panel.selected &&
         panel.selectedAction == FarmPlayerAction::Till;
    stage = 5;
    ok = ok && runtime.Input()->Push(interact, true) && runtime.Tick() &&
         runtime.FarmWorld()->Character().x == 1U && runtime.Farm()->TileStateAt(1U, 0U) == FarmTileState::Tilled;

    stage = 6;
    ok = ok && runtime.Input()->Push(interact, false) && runtime.Tick() && runtime.RenderFarm();
    const RuntimeFarmRenderReceipt* actionRender = runtime.LastFarmRenderReceipt();
    ok = ok && actionRender != nullptr && actionRender->frame == 2U && actionRender->presentedFrameCount == 2U;

    const std::vector<uint8_t> savedWorld = runtime.FarmWorld()->Serialize();
    const RuntimeTimeSnapshot savedTime = runtime.Time()->Snapshot();
    std::vector<uint8_t> checkpoint;
    stage = 7;
    ok = ok && runtime.SaveFarmProgressCheckpoint(42U, checkpoint) && !checkpoint.empty();

    stage = 8;
    ok = ok && runtime.Input()->Push(right, true) && runtime.Tick() &&
         runtime.FarmWorld()->Character().x == 2U && runtime.Farm()->TileStateAt(2U, 0U) == FarmTileState::Empty &&
         runtime.Input()->Push(right, false) && runtime.Tick();
    stage = 9;
    ok = ok && runtime.Input()->Push(interact, true) && runtime.Tick() &&
         runtime.Farm()->TileStateAt(2U, 0U) == FarmTileState::Tilled;
    ok = ok && runtime.Input()->Push(interact, false);

    uint64_t restoredRevision = 0U;
    stage = 10;
    ok = ok && runtime.RestoreFarmProgressCheckpoint(checkpoint, restoredRevision) && restoredRevision == 42U &&
         runtime.FarmWorld()->Serialize() == savedWorld && runtime.FarmWorld()->Character().x == 1U &&
         runtime.Farm()->TileStateAt(1U, 0U) == FarmTileState::Tilled &&
         runtime.Time()->Snapshot().gameTimeUnits == savedTime.gameTimeUnits &&
         runtime.LastFrameReceipt() == nullptr && runtime.LastFarmRenderReceipt() == nullptr;

    std::vector<uint8_t> corrupted = checkpoint;
    if (!corrupted.empty()) corrupted.back() ^= 0xA5U;
    stage = 11;
    ok = ok && !runtime.RestoreFarmProgressCheckpoint(corrupted, restoredRevision) &&
         runtime.LastError() == RuntimeError::CheckpointDecodeFailed;

    ok = ok && runtime.Shutdown();
    if (!ok) {
        std::fprintf(stderr, "FARM_VERTICAL_SLICE_SMOKE_FAIL stage=%d error=%u\n", stage, static_cast<unsigned>(runtime.LastError()));
        return 1;
    }
    std::printf("FARM_VERTICAL_SLICE_SMOKE_OK input=1 action=1 render=2 present=2 checkpoint=1 restore=1 corrupt_reject=1\n");
    return 0;
}
