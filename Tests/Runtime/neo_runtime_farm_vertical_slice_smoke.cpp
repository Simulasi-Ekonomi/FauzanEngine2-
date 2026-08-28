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
    config.farmBalance.maxEnergy = 4U;
    config.farmBalance.energyRegenPerTick = 2U;
    config.farmBalance.tillEnergyCost = 1U;
    config.farmBalance.plantEnergyCost = 2U;
    config.farmBalance.waterEnergyCost = 1U;
    config.farmBalance.harvestEnergyCost = 2U;
    config.farmBalance.growthTicks = {6U, 8U, 10U};
    config.farmBalance.harvestYield = {3U, 2U, 2U};
    config.farmBalance.sellPrice = {7LL, 8LL, 12LL};

    NeoRuntime runtime;
    if (!runtime.Initialize(config) || runtime.Farm() == nullptr || runtime.FarmWorld() == nullptr || runtime.Input() == nullptr ||
        runtime.FarmPlayerInput() == nullptr || runtime.Curriculum() == nullptr || runtime.SurfacePresenter() == nullptr || !runtime.SurfacePresenter()->IsReady()) return 1;

    const auto selectAndInteract = [&](FarmPlayerAction expected, FarmTileState expectedBefore, FarmTileState expectedAfter) {
        FarmActionPanelReceipt receipt{};
        const bool rendered = runtime.RenderFarm();
        const bool tabbed = rendered && runtime.RouteFarmHudKeyboard(UiKeyboardKey::TabForward, receipt) && !receipt.selected;
        const bool activated = tabbed && runtime.RouteFarmHudKeyboard(UiKeyboardKey::Activate, receipt) && receipt.selected && receipt.selectedAction == expected;
        const bool selected = activated && runtime.FarmPlayerInput()->SelectedAction() == expected;
        const bool before = selected && runtime.Farm()->TileStateAt(0U, 0U) == expectedBefore;
        const bool pressed = before && runtime.Input()->Push(MakeInputCode(InputDeviceType::Keyboard, 24U), true);
        const bool ticked = pressed && runtime.Tick();
        const bool after = ticked && runtime.Farm()->TileStateAt(0U, 0U) == expectedAfter;
        const bool receiptReady = after && runtime.LastFrameReceipt() != nullptr && runtime.LastFrameReceipt()->hasCurriculumReceipt;
        const bool released = receiptReady && runtime.Input()->Push(MakeInputCode(InputDeviceType::Keyboard, 24U), false);
        const bool finalTick = released && runtime.Tick();
        if (!finalTick) std::fprintf(stderr, "R2_ACTION_DIAG expected=%u rendered=%d tabbed=%d activated=%d selected=%d before=%d pressed=%d ticked=%d after=%d receipt=%d released=%d runtime_error=%u farm_error=%u tile=%u energy=%u focus=%u\n", static_cast<unsigned>(expected), rendered ? 1 : 0, tabbed ? 1 : 0, activated ? 1 : 0, selected ? 1 : 0, before ? 1 : 0, pressed ? 1 : 0, ticked ? 1 : 0, after ? 1 : 0, receiptReady ? 1 : 0, released ? 1 : 0, static_cast<unsigned>(runtime.LastError()), static_cast<unsigned>(runtime.Farm()->LastError()), static_cast<unsigned>(runtime.Farm()->TileStateAt(0U, 0U)), runtime.Farm()->Energy(), static_cast<unsigned>(receipt.selectedAction));
        return finalTick;
    };

    bool ok = selectAndInteract(FarmPlayerAction::Till, FarmTileState::Empty, FarmTileState::Tilled) &&
              selectAndInteract(FarmPlayerAction::PlantWheat, FarmTileState::Tilled, FarmTileState::Growing) &&
              selectAndInteract(FarmPlayerAction::Water, FarmTileState::Growing, FarmTileState::Growing);
    for (uint8_t tick = 0U; ok && tick < 9U; ++tick) ok = runtime.Tick();
    ok = ok && runtime.Farm()->TileStateAt(0U, 0U) == FarmTileState::Harvestable &&
         runtime.Farm()->Energy() == 4U &&
         selectAndInteract(FarmPlayerAction::Harvest, FarmTileState::Harvestable, FarmTileState::Empty) &&
         runtime.LastFrameReceipt() != nullptr && runtime.LastFrameReceipt()->hasCurriculumReceipt && runtime.LastFrameReceipt()->curriculum.completedLessons >= 2U;
    const uint32_t energyAfterHarvest = runtime.Farm()->Energy();
    const bool budgetActions = energyAfterHarvest == 4U && runtime.Farm()->Till(1U, 0U) && runtime.Farm()->Plant(1U, 0U, FarmCrop::Wheat) && runtime.Farm()->Water(1U, 0U);
    const uint32_t seedsAfterBudgetActions = runtime.Farm()->ItemCount(FarmItem::WheatSeed);
    const bool budgetDepleted = budgetActions && runtime.Farm()->Energy() == 0U;
    const bool rejectedForEnergy = budgetDepleted && !runtime.Farm()->Till(2U, 0U) && runtime.Farm()->LastError() == FarmError::InsufficientEnergy;
    const bool preservedAfterEnergyFailure = rejectedForEnergy && runtime.Farm()->TileStateAt(2U, 0U) == FarmTileState::Empty && runtime.Farm()->ItemCount(FarmItem::WheatSeed) == seedsAfterBudgetActions;
    const bool recovered = preservedAfterEnergyFailure && runtime.Farm()->Tick(1U) && runtime.Farm()->Energy() == 2U && runtime.Farm()->Till(2U, 0U);
    const bool economy = recovered && runtime.Farm()->SellCrop(77U, FarmCrop::Wheat, 3U) && runtime.Farm()->Coins() == 121 && runtime.Farm()->SellCrop(78U, FarmCrop::Wheat, 1U) == false && runtime.Farm()->LastError() == FarmError::InsufficientInventory && runtime.Farm()->Coins() == 121;
    if (!budgetActions || !economy) std::fprintf(stderr, "R2_BALANCE_DIAG post_harvest_energy=%u budget_actions=%d depleted=%d rejected=%d preserved=%d recovered=%d economy=%d energy=%u coins=%lld seeds=%u produce=%u tile1=%u tile2=%u farm_error=%u\n", energyAfterHarvest, budgetActions ? 1 : 0, budgetDepleted ? 1 : 0, rejectedForEnergy ? 1 : 0, preservedAfterEnergyFailure ? 1 : 0, recovered ? 1 : 0, economy ? 1 : 0, runtime.Farm()->Energy(), static_cast<long long>(runtime.Farm()->Coins()), runtime.Farm()->ItemCount(FarmItem::WheatSeed), runtime.Farm()->ItemCount(FarmItem::WheatProduce), static_cast<unsigned>(runtime.Farm()->TileStateAt(1U, 0U)), static_cast<unsigned>(runtime.Farm()->TileStateAt(2U, 0U)), static_cast<unsigned>(runtime.Farm()->LastError()));
    ok = ok && budgetActions && economy;
    LessonProgress orientation{};
    ok = ok && runtime.Curriculum()->Query("agri.orientation", orientation) && orientation.status == LessonStatus::Completed &&
         runtime.Curriculum()->Query("agri.land-investment", orientation) && orientation.status == LessonStatus::Completed &&
         runtime.Farm()->ItemCount(FarmItem::WheatSeed) == 30U && runtime.Farm()->ItemCount(FarmItem::WheatProduce) == 0U &&
         runtime.RenderFarm() && runtime.LastFarmRenderReceipt() != nullptr && runtime.LastFarmRenderReceipt()->worldFramebufferHash != 0U &&
         runtime.LastFarmRenderReceipt()->hudFramebufferHash != 0U && runtime.LastFarmRenderReceipt()->worldFramebufferHash != runtime.LastFarmRenderReceipt()->hudFramebufferHash &&
         runtime.SurfacePresenter()->PresentedFrameCount() != 0U && runtime.SurfacePresenter()->LastPresentedHash() == runtime.Renderer()->FrameHash();
    if (!ok) {
        std::fprintf(stderr, "NEO_RUNTIME_FARM_VERTICAL_SLICE_CURRICULUM_DIAG completed=%u events=%zu has_receipt=%d runtime_error=%u farm_error=%u energy=%u/%u tile=%u seeds=%u\n",
            runtime.Curriculum() == nullptr ? 0U : runtime.Curriculum()->LastReceipt().completedLessons,
            runtime.LastCurriculumEvents().size(), runtime.LastFrameReceipt() != nullptr && runtime.LastFrameReceipt()->hasCurriculumReceipt ? 1 : 0,
            static_cast<unsigned>(runtime.LastError()), runtime.Farm() == nullptr ? 255U : static_cast<unsigned>(runtime.Farm()->LastError()),
            runtime.Farm() == nullptr ? 0U : runtime.Farm()->Energy(), runtime.Farm() == nullptr ? 0U : runtime.Farm()->MaxEnergy(),
            runtime.Farm() == nullptr ? 255U : static_cast<unsigned>(runtime.Farm()->TileStateAt(0U, 0U)), runtime.Farm() == nullptr ? 0U : runtime.Farm()->ItemCount(FarmItem::WheatSeed));
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
    std::printf("NEO_RUNTIME_FARM_VERTICAL_SLICE_SMOKE_OK actions=till,plant,water,harvest hud=1 curriculum=onboarding_progression balance=authored energy=failure-recovery checkpoint=atomic cpu_present=1\n");
    return 0;
}
