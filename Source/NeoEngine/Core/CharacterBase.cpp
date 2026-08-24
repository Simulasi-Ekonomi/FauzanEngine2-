#include "CharacterBase.h"
#include <cmath>

namespace NeoEngine {

void CharacterBase::MoveForward(float value) {
    AddLocalOffset(GetForwardVector() * (speed * value * 0.016f));
}

void CharacterBase::MoveRight(float value) {
    AddLocalOffset(GetRightVector() * (speed * value * 0.016f));
}

void CharacterBase::Jump() {
    AddWorldOffset(Vector3(0, 5.0f, 0));
}

void CharacterBase::Tick(float deltaTime) {
    if (GetLocation().y > 0.0f) {
        AddWorldOffset(Vector3(0, -9.8f * deltaTime, 0));
        if (GetLocation().y < 0.0f) SetActorLocation(Vector3(GetLocation().x, 0, GetLocation().z));
    }
}

} // namespace NeoEngine
