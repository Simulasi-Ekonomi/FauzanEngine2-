#pragma once
#include <string>
#include <functional>
namespace NeoEngine {
class Platform {
public:
    static Platform& Get();
    virtual void Init()=0; virtual void Shutdown()=0; virtual void PumpEvents()=0;
    virtual std::string GetPlatformName()const=0; virtual uint64_t GetTimeNano()const=0;
    virtual void SetMainLoopCallback(std::function<void(float)>cb)=0;
};
}
