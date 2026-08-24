#pragma once
#include <vector>
#include <string>
#include <functional>
#include <memory>

namespace NeoEngine {

class ActorBase;
class CharacterBase;
class PlayerController; // Forward declaration

class GameModeBase {
public:
    GameModeBase() = default;
    virtual ~GameModeBase() = default;
    virtual void InitGame() {}
    virtual void StartPlay() {}
    virtual bool HasMatchStarted() const { return true; }
    virtual void PostLogin(PlayerController* pc) {}
    virtual void Logout(PlayerController* pc) {}
    virtual void RestartPlayer(PlayerController* pc) {}
    virtual void BeginPlay() {}
    virtual PlayerController* SpawnPlayerController() { return nullptr; }
    virtual CharacterBase* SpawnDefaultPawnFor(PlayerController* pc) { return nullptr; }
    virtual void EndPlay(const std::string& reason) {}
    virtual void Tick(float deltaTime) {}
    virtual bool ShouldTick() const { return true; }
    virtual void ScoreKill(PlayerController* killer, PlayerController* victim) {}
    virtual void AddScore(PlayerController* pc, int points) {}
    virtual void OnMatchStateSet(const std::string& state) {}
    virtual void HandleMatchIsWaitingToStart() {}
    virtual void HandleMatchHasStarted() {}
    virtual void HandleMatchHasEnded() {}
    virtual void HandleLeavingMap() {}
    virtual bool ReadyToStartMatch() { return true; }
    virtual bool ReadyToEndMatch() { return false; }
    std::function<void(PlayerController*)> OnPlayerJoined;
    std::function<void(PlayerController*)> OnPlayerLeft;

protected:
    std::vector<PlayerController*> players_;
};

} // namespace NeoEngine
