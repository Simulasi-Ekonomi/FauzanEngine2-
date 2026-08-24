#pragma once

namespace NeoEngine
{

class ISystem
{
public:
    virtual ~ISystem() = default;
    virtual void Update()         {}
    virtual void Update(float dt) { (void)dt; }
};

} // namespace NeoEngine
