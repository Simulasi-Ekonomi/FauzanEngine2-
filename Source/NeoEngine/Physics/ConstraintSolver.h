#pragma once
#include "RigidBody.h"

namespace NeoEngine {

class ConstraintSolver {
public:
    static void Resolve(RigidBody& a, RigidBody& b);
};

}
