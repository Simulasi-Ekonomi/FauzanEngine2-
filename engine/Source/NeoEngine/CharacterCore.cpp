#include "CharacterCore.h"
#include <cmath>

void ACharacterCore::Move(float AxisValue) {
    // Update posisi berdasarkan vektor depan (Forward Vector)
    Position[0] += std::cos(Rotation[1]) * AxisValue * WalkSpeed * 0.016f;
    Position[2] += std::sin(Rotation[1]) * AxisValue * WalkSpeed * 0.016f;
}

void ACharacterCore::Jump() {
    // Simple Z-axis impulse
    Position[1] += 5.0f; 
}