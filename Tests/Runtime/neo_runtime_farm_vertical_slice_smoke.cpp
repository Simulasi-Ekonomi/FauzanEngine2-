#include "Runtime/NeoRuntime.h"

#include <cstdio>
#include <vector>

using namespace NeoEngine;

int main() {
    RuntimeConfig config{};
    config.farmWidth = 4U;
    config.farmHeight = 4U;
    config.farmNpcCount = 2U;
    config.renderWidth = 128U;
    config.renderHeight = 96U;
    config.enableFarmPlayerInput = true;
    config.enableFarmRuntimeHud = true;
    config.enableSoftwareSurfacePresentation = true;
    config.softwareSurfaceHidden = true;
    config.enableFarmCurriculum = true;

    NeoRuntime runtime;
    if (!runtime.Initialize(config) || runtime.Farm() == nullptr || runtime.FarmWorld() == nullptr || runtime.Input() == nullptr ||
        runtime.FarmPlayerInput() == nullptr || runtime.Curriculum() == nullptr || runtime.SurfacePresenter() == nullptr || !runtime.SurfacePresenter()->IsReady()) return 1;

    const auto selectAndInteract = [&](FarmPlayerAction expected, FarmTileState expectedBefore, FarmTileState expectedAfter) {
        FarmActionPanelReceipt receipt{};
        if (!runtime.RenderFarm()) return false;
        if (!runtime.RouteFarmHudKeyboard(UiKeyboardKey::TabForward, receipt) || receipt.selected ||
            !runtime.RouteFarmHudKeyboard(UiKeyboardKey::Activate, receipt) || !receipt.selected || receipt.selectedAction != expected ||
            runtime.FarmPlayerInput()->SelectedAction() != expected || runtime.Farm()->TileStateAt(0U, 0U) != expectedBefore ||
            !runtime.Input()->Push(MakeInputCode(InputDeviceType::Keyboard, 24U), true) || !runtime.Tick() ||
            runtime.Farm()->TileStateAt(0U, 0U) != expectedAfter || runtime.LastFrameReceipt() == nullptr || !runtime.LastFrameReceipt()->hasCurriculumReceipt ||
            !runtime.Input()->Push(MakeInputCode(InputDeviceType::Keyboard, 24U), false) || !runtime.Tick()) return false;
        return true;
    };

    bool ok = selectAndInteract(FarmPlayerAction::Till, FarmTileState::Empty, FarmTileState::Tilled) &&
              selectAndInteract(FarmPlayerAction::PlantWheat, FarmTileState::Tilled, FarmTileState::Growing) &&
              selectAndInteract(FarmPlayerAction::Water, FarmTileState::Growing, FarmTileState::Growing);
    for (uint8_t tick = 0U; ok && tick < 9U; ++tick) ok = runtime.Tick();
    ok = ok && runtime.Farm()->TileStateAt(0U, 0U) == FarmTileState::Harvestable &&
         selectAndInteract(FarmPlayerAction::Harvest, FarmTileState::Harvestable, FarmTileState::Empty) &&
         runtime.LastFrameReceipt() != nullptr && runtime.LastFrameReceipt()->hasCurriculumReceipt && runtime.LastFrameReceipt()->curriculum.completedLessons >= 2U;
    LessonProgress orientation{};
    ok = ok && runtime.Curriculum()->Query("agri.orientation", orientation) && orientation.status == LessonStatus::Completed &&
         runtime.Curriculum()->Query("agri.land-investment", orientation) && orientation.status == LessonStatus::Completed &&
         runtime.Farm()->ItemCount(FarmItem::WheatSeed) == 31U && runtime.Farm()->ItemCount(FarmItem::WheatProduce) == 2U &&
         runtime.RenderFarm() && runtime.LastFarmRenderReceipt() != nullptr && runtime.LastFarmRenderReceipt()->worldFramebufferHash != 0U &&
         runtime.LastFarmRenderReceipt()->hudFramebufferHash != 0U && runtime.LastFarmRenderReceipt()->worldFramebufferHash != runtime.LastFarmRenderReceipt()->hudFramebufferHash &&
         runtime.SurfacePresenter()->PresentedFrameCount() != 0U && runtime.SurfacePresenter()->LastPresentedHash() == runtime.Renderer()->FrameHash();
    if (!ok) {
        std::fprintf(stderr, "NEO_RUNTIME_FARM_VERTICAL_SLICE_CURRICULUM_DIAG completed=%u events=%zu has_receipt=%d\n",
            runtime.Curriculum() == nullptr ? 0U : runtime.Curriculum()->LastReceipt().completedLessons,
            runtime.LastCurriculumEvents().size(), runtime.LastFrameReceipt() != nullptr && runtime.LastFrameReceipt()->hasCurriculumReceipt ? 1 : 0);
        return 1;
    }

    std::vector<uint8_t> checkpoint;
    const std::vector<uint8_t> savedWorld = runtime.FarmWorld()->Serialize();
    const CurriculumProgressReceipt savedCurriculum = runtime.Curriculum()->LastReceipt();
    ok = ok && runtime.SaveFarmProgressCheckpoint(42U, checkpoint) && !checkpoint.empty() &&
         runtime.Input()->Push(MakeInputCode(InputDeviceType::Keyboard, 23U), true) && runtime.Tick() && runtime.FarmWorld()->Character().x == 1U &&
         runtime.Input()->Push(MakeInputCode(InputDeviceType::Keyboard, 23U), false) && runtime.Tick() && runtime.FarmWorld()->Serialize() != savedWorld;
    uint64_t restoredRevision = 0U;
    ok = ok && runtime.RestoreFarmProgressCheckpoint(checkpoint, restoredRevision) && restoredRevision == 42U &&
         runtime.FarmWorld()->Serialize() == savedWorld && runtime.Curriculum() != nullptr && runtime.Curriculum()->LastReceipt().completedLessons == savedCurriculum.completedLessons &&
         runtime.Curriculum()->LastReceipt().revision == savedCurriculum.revision && runtime.LastFrameReceipt() == nullptr && runtime.LastFarmRenderReceipt() == nullptr && runtime.RenderFarm() &&
         runtime.LastFarmRenderReceipt() != nullptr && runtime.LastFarmRenderReceipt()->hudFramebufferHash != 0U;

    const std::vector<uint8_t> preservedWorld = runtime.FarmWorld()->Serialize();
    const RuntimeFarmRenderReceipt preservedRender = *runtime.LastFarmRenderReceipt();
    std::vector<uint8_t> corrupt = checkpoint;
    corrupt.back() ^= 0x01U;
    uint64_t ignoredRevision = 0U;
    ok = ok && !runtime.RestoreFarmProgressCheckpoint(corrupt, ignoredRevision) && runtime.LastError() == RuntimeError::CheckpointDecodeFailed &&
         runtime.FarmWorld()->Serialize() == preservedWorld && runtime.LastFarmRenderReceipt() != nullptr &&
         runtime.LastFarmRenderReceipt()->worldFramebufferHash == preservedRender.worldFramebufferHash &&
         runtime.LastFarmRenderReceipt()->hudFramebufferHash == preservedRender.hudFramebufferHash && runtime.Curriculum() != nullptr &&
         runtime.Curriculum()->LastReceipt().completedLessons == savedCurriculum.completedLessons && runtime.Shutdown();
    if (!ok) {
        std::fprintf(stderr, "NEO_RUNTIME_FARM_VERTICAL_SLICE_SMOKE_FAIL\n");
        return 1;
    }
    std::printf("NEO_RUNTIME_FARM_VERTICAL_SLICE_SMOKE_OK actions=till,plant,water,harvest hud=1 curriculum=onboarding_progression checkpoint=atomic cpu_present=1\n");
    return 0;
}
