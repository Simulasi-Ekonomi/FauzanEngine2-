#pragma once
#include "../Common/Platform.h"
class AndroidPlatform : public Platform {
public:
    void Init() override;
    void PumpEvents() override;
    void Shutdown() override;
};
