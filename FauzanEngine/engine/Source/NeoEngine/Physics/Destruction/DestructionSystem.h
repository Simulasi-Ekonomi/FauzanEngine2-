#pragma once
#include <vector>

struct DestructibleObject
{
    float x;
    float y;
    float z;
    float health;
};

class DestructionSystem
{
public:

    void AddObject(const DestructibleObject& obj);

    void ApplyDamage(int index,float dmg);

    const std::vector<DestructibleObject>& Objects() const;

private:

    std::vector<DestructibleObject> objects;
};
