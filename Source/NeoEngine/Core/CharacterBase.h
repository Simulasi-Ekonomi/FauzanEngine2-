#pragma once
#include "Core/ActorBase.h"

namespace NeoEngine {

class CharacterBase : public ActorBase {
public:
    CharacterBase() = default;
    virtual ~CharacterBase() = default;

    void MoveForward(float value);
    void MoveRight(float value);
    void Jump();

    float speed = 500.0f;  // unit per detik

    virtual void Tick(float deltaTime) override;
};

} // namespace NeoEngine
