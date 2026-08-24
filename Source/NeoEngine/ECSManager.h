#pragma once
#include <vector>

struct FVector3 {
    float x, y, z;
};

class UECSManager {
public:
    static const size_t MAX_ENTITIES = 10000;
    size_t EntityCount = 0;
    
    FVector3 Positions[MAX_ENTITIES];
    FVector3 Velocities[MAX_ENTITIES];

    void UpdateSystems(float DeltaTime);
};
