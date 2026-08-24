#pragma once

namespace NeoEngine
{

class Subsystem
{
public:
    virtual ~Subsystem() = default;

    virtual void Init() {}
    virtual void Tick() {}
    virtual void Shutdown() {}

    virtual bool IsInitialized() const { return true; }
};

}
