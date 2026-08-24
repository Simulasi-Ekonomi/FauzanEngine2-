#pragma once
#include <cstdint>

typedef uint64_t uint64;
typedef uint32_t uint32;

class Platform {
public:
    virtual ~Platform() = default;
    
    virtual void Init() = 0;
    virtual void PumpEvents() = 0;
    virtual void Shutdown() = 0;
    virtual const char* GetPlatformName() = 0;
    virtual uint64 GetTimeNano() = 0;
    virtual void Sleep(uint32 ms) = 0;
    
    static Platform* Create();
};
