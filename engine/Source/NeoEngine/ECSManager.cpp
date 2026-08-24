#include "ECSManager.h"

void UECSManager::UpdateSystems(float DeltaTime) {
    // Memproses ribuan entitas dalam satu loop cache-friendly
    for(size_t i = 0; i < EntityCount; ++i) {
        Positions[i].x += Velocities[i].x * DeltaTime;
        Positions[i].y += Velocities[i].y * DeltaTime;
        Positions[i].z += Velocities[i].z * DeltaTime;
    }
}
