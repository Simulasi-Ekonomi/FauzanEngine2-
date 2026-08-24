#include "DestructionSystem.h"

void DestructionSystem::AddObject(const DestructibleObject& obj)
{
    objects.push_back(obj);
}

void DestructionSystem::ApplyDamage(int index,float dmg)
{
    if(index >=0 && index < objects.size())
    {
        objects[index].health -= dmg;

        if(objects[index].health < 0)
            objects[index].health = 0;
    }
}

const std::vector<DestructibleObject>& DestructionSystem::Objects() const
{
    return objects;
}
