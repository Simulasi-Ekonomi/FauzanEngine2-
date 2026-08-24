#pragma once
#include "../NeoEngine/Core/Engine.h"

namespace MatchState {
    const char* EnteringMap = "EnteringMap";
    const char* WaitingToStart = "WaitingToStart";
    const char* InProgress = "InProgress";
    const char* WaitingPostMatch = "WaitingPostMatch";
}

class AGameStateBase {
public:
    AGameStateBase() : CurrentMatchState(MatchState::EnteringMap), ElapsedTime(0.0f) {}

    virtual void OnStepTick(float DeltaTime) {
        ElapsedTime += DeltaTime;
    }

    const char* GetMatchState() const { return CurrentMatchState; }

protected:
    const char* CurrentMatchState;
    float ElapsedTime;
};
