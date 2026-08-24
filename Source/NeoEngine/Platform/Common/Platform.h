#pragma once
class Platform {
public:
    virtual ~Platform() = default;
    virtual void Init() = 0;
    virtual void PumpEvents() = 0;
    virtual void Shutdown() = 0;
    static Platform& Get();
};
