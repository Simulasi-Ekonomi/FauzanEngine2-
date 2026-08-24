#pragma once
#include "../Platform.h"

class DesktopPlatform : public Platform {
public:
    void Init() override;
    void PumpEvents() override;
    void Shutdown() override;
};
