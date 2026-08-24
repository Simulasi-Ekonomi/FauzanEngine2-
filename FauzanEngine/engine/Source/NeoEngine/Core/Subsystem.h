#pragma once
class Subsystem {
public:
    virtual ~Subsystem() = default;
    virtual void Init() = 0;
    virtual void Tick() = 0;
    virtual void Shutdown() = 0;
};
