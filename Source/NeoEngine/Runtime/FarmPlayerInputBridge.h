#pragma once

#include "InputState.h"

#include <cstdint>
#include <string>

namespace NeoEngine {
class FarmWorldTool;

enum class FarmPlayerAction : uint8_t { Till, PlantWheat, Water, Harvest };
enum class FarmPlayerInputBridgeError : uint8_t { None, InvalidConfiguration, NotInitialized, MissingAction, ConflictingInput, InvalidCoordinate, WorldRejected };
struct FarmPlayerInputBindings { std::string moveUp = "farm_move_up"; std::string moveDown = "farm_move_down"; std::string moveLeft = "farm_move_left"; std::string moveRight = "farm_move_right"; std::string interact = "farm_interact"; };

// Maps a bounded local input snapshot to existing FarmWorldTool player APIs.
// It cannot sell, top up, issue permits, change bans, or mutate agent authority.
class FarmPlayerInputBridge {
public:
    bool Initialize(FarmPlayerInputBindings bindings = {});
    void SetSelectedAction(FarmPlayerAction action) { selectedAction_ = action; }
    bool Step(const InputState& input, FarmWorldTool& world);
    [[nodiscard]] FarmPlayerAction SelectedAction() const { return selectedAction_; }
    [[nodiscard]] FarmPlayerInputBridgeError LastError() const { return lastError_; }
    [[nodiscard]] bool IsReady() const { return initialized_; }
private:
    bool Fail(FarmPlayerInputBridgeError error);
    FarmPlayerInputBindings bindings_{};
    FarmPlayerAction selectedAction_ = FarmPlayerAction::Till;
    FarmPlayerInputBridgeError lastError_ = FarmPlayerInputBridgeError::NotInitialized;
    bool initialized_ = false;
};
} // namespace NeoEngine
