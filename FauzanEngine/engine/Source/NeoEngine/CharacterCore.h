#pragma once
#include "ActorCore.h"
#include <string>

class ACharacterCore : public AActorCore {
public:
    void Move(float AxisValue);
    void Jump();
    void SetupPlayerInputComponent();
    float WalkSpeed = 600.0f;
};