#pragma once
#include <cstdint>

class NeoTime {
public:
    static void Init();
    static void Update();
    static float DeltaTime();
    static double TotalTime();
    static uint64_t FrameCount();
};
