#pragma once
#include <cstdint>
struct FrameContext{
    uint64_t frameIndex = 0;
    double deltaTime = 0.0;
    bool ready = false;
};
