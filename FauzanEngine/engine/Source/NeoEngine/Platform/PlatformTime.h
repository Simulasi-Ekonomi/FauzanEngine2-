#pragma once
#include "PlatformTypes.h"

class PlatformTime {
public:
    // Sumber waktu tunggal untuk DeltaTime dan Profiling
    static uint64 NowNano();
    static double NowSeconds();
};
