#pragma once

class Registry;

class System
{
public:
    virtual ~System() = default;

    virtual void Update(float dt, Registry& registry) = 0;
};
