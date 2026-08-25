#include "Runtime/FarmPlayerInputBridge.h"

#include "Systems/FarmWorldTool.h"

#include <array>

namespace NeoEngine {
namespace { bool ValidBindings(const FarmPlayerInputBindings& bindings) { const std::array<std::string, 5> values{bindings.moveUp, bindings.moveDown, bindings.moveLeft, bindings.moveRight, bindings.interact}; for (const std::string& value : values) if (value.empty() || value.size() > 64U) return false; for (size_t first = 0; first < values.size(); ++first) for (size_t second = first + 1U; second < values.size(); ++second) if (values[first] == values[second]) return false; return true; } }
bool FarmPlayerInputBridge::Fail(FarmPlayerInputBridgeError error) { lastError_ = error; return false; }
bool FarmPlayerInputBridge::Initialize(FarmPlayerInputBindings bindings) { if (!ValidBindings(bindings)) return Fail(FarmPlayerInputBridgeError::InvalidConfiguration); bindings_ = std::move(bindings); initialized_ = true; lastError_ = FarmPlayerInputBridgeError::None; return true; }
bool FarmPlayerInputBridge::Step(const InputState& input, FarmWorldTool& world) {
    if (!initialized_) return Fail(FarmPlayerInputBridgeError::NotInitialized);
    if (!world.IsReady()) return Fail(FarmPlayerInputBridgeError::WorldRejected);
    const std::array<std::string, 5> required{bindings_.moveUp, bindings_.moveDown, bindings_.moveLeft, bindings_.moveRight, bindings_.interact};
    for (const std::string& action : required) if (!input.HasAction(action)) return Fail(FarmPlayerInputBridgeError::MissingAction);
    const bool up = input.Query(bindings_.moveUp).pressed, down = input.Query(bindings_.moveDown).pressed, left = input.Query(bindings_.moveLeft).pressed, right = input.Query(bindings_.moveRight).pressed, interact = input.Query(bindings_.interact).justPressed;
    const int8_t dx = left == right ? 0 : (right ? 1 : -1);
    const int8_t dz = up == down ? 0 : (down ? 1 : -1);
    if (interact && (dx != 0 || dz != 0)) return Fail(FarmPlayerInputBridgeError::ConflictingInput);
    const FarmCharacterState current = world.Character();
    if (dx != 0 || dz != 0) {
        const FarmWorldSnapshot snapshot = world.Snapshot();
        const int32_t nextX = static_cast<int32_t>(current.x) + dx, nextZ = static_cast<int32_t>(current.z) + dz;
        if (nextX < 0 || nextZ < 0 || nextX >= snapshot.worldWidth || nextZ >= snapshot.worldHeight) return Fail(FarmPlayerInputBridgeError::InvalidCoordinate);
        if (!world.SetCharacterState({static_cast<uint16_t>(nextX), static_cast<uint16_t>(nextZ), current.level})) return Fail(FarmPlayerInputBridgeError::WorldRejected);
    } else if (interact) {
        bool accepted = false;
        switch (selectedAction_) {
            case FarmPlayerAction::Till: accepted = world.PlayerTill(current.x, current.z); break;
            case FarmPlayerAction::PlantWheat: accepted = world.PlayerPlant(current.x, current.z, FarmCrop::Wheat); break;
            case FarmPlayerAction::Water: accepted = world.PlayerWater(current.x, current.z); break;
            case FarmPlayerAction::Harvest: { uint32_t harvested = 0; accepted = world.PlayerHarvest(current.x, current.z, harvested); break; }
        }
        if (!accepted) return Fail(FarmPlayerInputBridgeError::WorldRejected);
    }
    lastError_ = FarmPlayerInputBridgeError::None;
    return true;
}
} // namespace NeoEngine
