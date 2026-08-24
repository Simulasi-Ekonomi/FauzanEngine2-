#pragma once
#include <cstdint>

class Stats {
public:
    static void FrameTick();
    static uint64_t GetFrameCount();
};
