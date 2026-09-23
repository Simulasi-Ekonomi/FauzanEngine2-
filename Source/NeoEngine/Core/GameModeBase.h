#pragma once
#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace NeoEngine {

class CharacterBase;
class PlayerController;

class GameModeBase {
public:
    GameModeBase() = default;
    virtual ~GameModeBase() = default;

    virtual void InitGame() {
        players_.clear();
        playerScores_.clear();
        elapsedSeconds_ = 0.0;
        matchState_ = "WaitingToStart";
        matchStarted_ = false;
        matchEnded_ = false;
        initialized_ = true;
    }

    virtual void StartPlay() {
        if (!initialized_) InitGame();
        BeginPlay();
        if (ReadyToStartMatch()) {
            HandleMatchIsWaitingToStart();
            matchStarted_ = true;
            matchEnded_ = false;
            matchState_ = "InProgress";
            HandleMatchHasStarted();
        }
    }

    [[nodiscard]] virtual bool HasMatchStarted() const {
        return matchStarted_ && !matchEnded_;
    }

    virtual void PostLogin(PlayerController* pc) {
        if (pc == nullptr || std::find(players_.begin(), players_.end(), pc) != players_.end()) return;
        players_.push_back(pc);
        playerScores_.try_emplace(pc, 0);
        if (OnPlayerJoined) OnPlayerJoined(pc);
    }

    virtual void Logout(PlayerController* pc) {
        if (pc == nullptr) return;
        const auto it = std::find(players_.begin(), players_.end(), pc);
        if (it == players_.end()) return;
        players_.erase(it);
        playerScores_.erase(pc);
        if (OnPlayerLeft) OnPlayerLeft(pc);
    }

    virtual void RestartPlayer(PlayerController* pc) {
        if (pc == nullptr || std::find(players_.begin(), players_.end(), pc) == players_.end()) return;
        CharacterBase* pawn = SpawnDefaultPawnFor(pc);
        if (pawn != nullptr) lastSpawnedPawns_[pc] = pawn;
    }

    virtual void BeginPlay() {
        if (!initialized_) InitGame();
        matchState_ = "WaitingToStart";
    }

    // A generic GameMode cannot manufacture concrete controller/pawn types.
    // Derived game modes must provide the actual factories instead of returning
    // a fake/null capability.
    virtual PlayerController* SpawnPlayerController() = 0;
    virtual CharacterBase* SpawnDefaultPawnFor(PlayerController* pc) = 0;

    virtual void EndPlay(const std::string& reason) {
        if (matchStarted_ && !matchEnded_) {
            matchEnded_ = true;
            matchStarted_ = false;
            matchState_ = "Ended";
            HandleMatchHasEnded();
        }
        lastEndReason_ = reason;
        players_.clear();
        playerScores_.clear();
        lastSpawnedPawns_.clear();
    }

    virtual void Tick(float deltaTime) {
        if (!HasMatchStarted() || deltaTime <= 0.0) return;
        elapsedSeconds_ += deltaTime;
        if (ReadyToEndMatch()) {
            matchEnded_ = true;
            matchStarted_ = false;
            matchState_ = "Ended";
            HandleMatchHasEnded();
        }
    }

    [[nodiscard]] virtual bool ShouldTick() const { return initialized_ && !matchEnded_; }

    virtual void ScoreKill(PlayerController* killer, PlayerController* victim) {
        if (victim == nullptr) return;
        if (killer != nullptr && killer != victim) AddScore(killer, 1);
        AddScore(victim, 0);
    }

    virtual void AddScore(PlayerController* pc, int points) {
        if (pc == nullptr || points == 0) return;
        if (std::find(players_.begin(), players_.end(), pc) == players_.end()) return;
        playerScores_[pc] += points;
    }

    virtual void OnMatchStateSet(const std::string& state) {
        if (state.empty()) return;
        matchState_ = state;
        matchStarted_ = state == "InProgress";
        matchEnded_ = state == "Ended";
    }

    virtual void HandleMatchIsWaitingToStart() {
        matchState_ = "WaitingToStart";
        matchStarted_ = false;
        matchEnded_ = false;
    }

    virtual void HandleMatchHasStarted() {
        matchState_ = "InProgress";
        matchStarted_ = true;
        matchEnded_ = false;
    }

    virtual void HandleMatchHasEnded() {
        matchState_ = "Ended";
        matchStarted_ = false;
        matchEnded_ = true;
    }

    virtual void HandleLeavingMap() {
        matchState_ = "LeavingMap";
        matchStarted_ = false;
    }

    [[nodiscard]] virtual bool ReadyToStartMatch() const {
        return initialized_ && !matchStarted_ && !matchEnded_;
    }

    [[nodiscard]] virtual bool ReadyToEndMatch() const {
        return false;
    }

    [[nodiscard]] const std::vector<PlayerController*>& Players() const { return players_; }

    [[nodiscard]] int Score(PlayerController* pc) const {
        if (pc == nullptr) return 0;
        const auto it = playerScores_.find(pc);
        return it == playerScores_.end() ? 0 : it->second;
    }

    [[nodiscard]] const std::string& MatchState() const { return matchState_; }
    [[nodiscard]] double ElapsedSeconds() const { return elapsedSeconds_; }
    [[nodiscard]] const std::string& LastEndReason() const { return lastEndReason_; }

    std::function<void(PlayerController*)> OnPlayerJoined;
    std::function<void(PlayerController*)> OnPlayerLeft;

protected:
    std::vector<PlayerController*> players_;
    std::unordered_map<PlayerController*, int> playerScores_;
    std::unordered_map<PlayerController*, CharacterBase*> lastSpawnedPawns_;
    std::string matchState_{"WaitingToStart"};
    std::string lastEndReason_;
    double elapsedSeconds_{0.0};
    bool initialized_{false};
    bool matchStarted_{false};
    bool matchEnded_{false};
};

} // namespace NeoEngine
