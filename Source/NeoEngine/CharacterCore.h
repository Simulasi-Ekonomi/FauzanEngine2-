#include "Core/Math/NeoMath.h"
#pragma once

#include "Core/AActorCore.h"
#include <string>

namespace NeoEngine {

class ACharacterCore : public AActorCore {
public:
    // Konstruktor & Destruktor eksplisit (diimplementasikan di .cpp)
    ACharacterCore();
    virtual ~ACharacterCore();

    // Gerakan dasar
    void MoveForward(float value);
    void Jump();
};

} // namespace NeoEngine
