#pragma once
#include "../Platform.h"

namespace NeoEngine {

class DesktopPlatform : public Platform {
public:
    void Init() override;
    void PumpEvents() override;
    void Shutdown() override;
};

} // namespace NeoEngine
