#include "Runtime/FarmPlayerInputBridge.h"
#include "Systems/FarmSystem.h"
#include "Systems/FarmWorldTool.h"
#include "Systems/TrustSafetySystem.h"

#include <cstdio>

namespace {
constexpr int32_t kUp = 1, kDown = 2, kLeft = 3, kRight = 4, kInteract = 5;
bool Frame(NeoEngine::InputState& input, int32_t code, bool pressed) { return input.Push(code, pressed), input.BeginFrame(), true; }
}

int main() {
    using namespace NeoEngine;
    const auto require = [](bool condition, const char* stage) { if (!condition) std::fprintf(stderr, "FARM_PLAYER_INPUT_BRIDGE_SMOKE_FAIL stage=%s\n", stage); return condition; };
    FarmSystem farm(4, 4, 100);
    TrustSafetySystem trust;
    FarmWorldTool world;
    FarmWorldConfig config{};
    config.worldWidth = 4;
    config.worldHeight = 4;
    config.npcCount = 1;
    if (!require(world.Initialize(farm, trust, "input-farm-player", config) && world.SetCharacterState({1, 1, 2}), "world-init")) return 1;
    InputState input;
    if (!require(input.Bind("farm_move_up", kUp) && input.Bind("farm_move_down", kDown) && input.Bind("farm_move_left", kLeft) && input.Bind("farm_move_right", kRight) && input.Bind("farm_interact", kInteract), "input-bind")) return 1;
    FarmPlayerInputBridge bridge;
    if (!require(bridge.Initialize(), "bridge-init")) return 1;
    if (!require(Frame(input, kRight, true) && bridge.Step(input, world) && world.Character().x == 2U && world.Character().z == 1U && bridge.LastReceipt().kind == FarmPlayerInputKind::Movement && bridge.LastReceipt().x == 2U && bridge.LastReceipt().z == 1U, "move-right")) return 1;
    if (!require(Frame(input, kRight, false) && bridge.Step(input, world) && Frame(input, kInteract, true) && bridge.Step(input, world) && farm.TileStateAt(2, 1) == FarmTileState::Tilled && bridge.LastReceipt().kind == FarmPlayerInputKind::Action && bridge.LastReceipt().action == FarmPlayerAction::Till && bridge.LastReceipt().harvestedUnits == 0U, "till")) return 1;
    if (!require(Frame(input, kInteract, false) && bridge.Step(input, world), "release-till")) return 1;
    bridge.SetSelectedAction(FarmPlayerAction::PlantWheat);
    if (!require(Frame(input, kInteract, true) && bridge.Step(input, world) && farm.TileStateAt(2, 1) == FarmTileState::Growing, "plant")) return 1;
    if (!require(Frame(input, kInteract, false) && bridge.Step(input, world), "release-plant")) return 1;
    bridge.SetSelectedAction(FarmPlayerAction::Water);
    if (!require(Frame(input, kInteract, true) && bridge.Step(input, world) && farm.Tick(3), "water")) return 1;
    if (!require(Frame(input, kInteract, false) && bridge.Step(input, world), "release-water")) return 1;
    const FarmCharacterState preserved = world.Character(); const FarmPlayerInputReceipt preservedReceipt = bridge.LastReceipt();
    if (!require(Frame(input, kLeft, true) && input.Push(kInteract, true) && (input.BeginFrame(), !bridge.Step(input, world)) && bridge.LastError() == FarmPlayerInputBridgeError::ConflictingInput && world.Character().x == preserved.x && world.Character().z == preserved.z && bridge.LastReceipt().kind == preservedReceipt.kind && bridge.LastReceipt().action == preservedReceipt.action && bridge.LastReceipt().x == preservedReceipt.x && bridge.LastReceipt().z == preservedReceipt.z && bridge.LastReceipt().harvestedUnits == preservedReceipt.harvestedUnits, "conflict")) return 1;
    if (!require(Frame(input, kLeft, false) && input.Push(kInteract, false) && (input.BeginFrame(), bridge.Step(input, world)), "release-conflict")) return 1;
    if (!require(world.SetCharacterState({0, 0, 2}) && Frame(input, kUp, true) && !bridge.Step(input, world) && bridge.LastError() == FarmPlayerInputBridgeError::InvalidCoordinate && world.Character().x == 0U && world.Character().z == 0U, "bounds")) return 1;
    InputState incomplete;
    if (!require(incomplete.Bind("farm_move_up", kUp) && !bridge.Step(incomplete, world) && bridge.LastError() == FarmPlayerInputBridgeError::MissingAction, "missing-action")) return 1;
    std::printf("FARM_PLAYER_INPUT_BRIDGE_SMOKE_OK movement=1 receipt=1 farmActions=3 conflict=1 bounds=1 noEconomyAuthority=1\n");
    return 0;
}
