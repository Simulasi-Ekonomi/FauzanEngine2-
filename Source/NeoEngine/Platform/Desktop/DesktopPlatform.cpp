#include "DesktopPlatform.h"
#include <iostream>

namespace NeoEngine {

void DesktopPlatform::Init() {
    std::cout << "Desktop Platform Layer Initialized." << std::endl;
}

void DesktopPlatform::PumpEvents() {
    // Poll events untuk GLFW/SDL di desktop
}

void DesktopPlatform::Shutdown() {
    std::cout << "Desktop Platform Layer Shutdown." << std::endl;
}

} // namespace NeoEngine