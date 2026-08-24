#pragma once

#include "ISystem.h"
#include "Registry.h"

namespace NeoEngine {

class MovementSystem : public ISystem {
public:
    void Update(float dt) override;
};

}
