#pragma once

#include "InputState.h"

#include <cstdint>
#include <string>

namespace NeoEngine {
class FarmWorldTool;

enum class FarmPlayerAction : uint8_t { Till, PlantWheat, Water, Harvest };
enum class FarmPlayerInputBridgeError : uint8_t { None, InvalidConfiguration, NotInitialized, MissingAction, ConflictingInput, InvalidCoordinate, WorldRejected };
enum class FarmPlayerInputKind : uint8_t { None, Movement, Action };
struct FarmPlayerInputBindings { std::string moveUp = "farm_move_up"; std::string moveDown = "farm_move_down"; std::string moveLeft = "farm_move_left"; std::string moveRight = "farm_move_right"; std::string interact = "farm_interact"; };
struct FarmPlayerInputReceipt { FarmPlayerInputKind kind = FarmPlayerInputKind::None; FarmPlayerAction action = FarmPlayerAction::Till; uint16_t x = 0U; uint16_t z = 0U; uint32_t harvestedUnits = 0U; };

// Maps a bounded local input snapshot to existing FarmWorldTool player APIs.
// It cannot sell, top up, issue permits, change bans, or mutate agent authority.
class FarmPlayerInputBridge {
public:
    bool Initialize(FarmPlayerInputBindings bindings = {});
    void SetSelectedAction(FarmPlayerAction action) { selectedAction_ = action; }
    bool Step(const InputState& input, FarmWorldTool& world);
    [[nodiscard]] FarmPlayerAction SelectedAction() const { return selectedAction_; }
    [[nodiscard]] FarmPlayerInputReceipt LastReceipt() const { return lastReceipt_; }
    [[nodiscard]] FarmPlayerInputBridgeError LastError() const { return lastError_; }
    [[nodiscard]] bool IsReady() const { return initialized_; }
private:
    bool Fail(FarmPlayerInputBridgeError error);
    FarmPlayerInputBindings bindings_{};
    FarmPlayerAction selectedAction_ = FarmPlayerAction::Till;
    FarmPlayerInputReceipt lastReceipt_{};
    FarmPlayerInputBridgeError lastError_ = FarmPlayerInputBridgeError::NotInitialized;
    bool initialized_ = false;
};
} // namespace NeoEngine
