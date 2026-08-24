#include "World.h"

void World::Update(float deltaTime) {
    Systems.UpdateAll(deltaTime);
}
