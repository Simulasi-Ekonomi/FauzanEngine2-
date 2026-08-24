#include "PlayerControllerBase.h"
#include "Core/Math/Quaternion.h"
#include <cmath>

namespace NeoEngine {

void PlayerController::Possess(ActorBase* pawn) {
    m_Pawn = pawn;
}

void PlayerController::ProcessLookX(float value) {
    if (!m_Pawn) return;
    Quaternion deltaRotation;
    float halfAngle = value * 0.005f * 0.5f;
    deltaRotation.w = NeoEngine::Math::Cos(halfAngle);
    deltaRotation.y = NeoEngine::Math::Sin(halfAngle);
    m_Pawn->AddActorWorldRotation(deltaRotation);
}

void PlayerController::ProcessLookY(float value) {
    // Implementasi pitch rotation (optional)
}

} // namespace NeoEngine
