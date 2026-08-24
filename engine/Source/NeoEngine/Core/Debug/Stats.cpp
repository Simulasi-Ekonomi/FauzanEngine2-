#include <cassert>
#include "Stats.h"

static uint64_t GFrameCount = 0;

void Stats::FrameTick() {
    ++GFrameCount;
}

uint64_t Stats::GetFrameCount() {
    return GFrameCount;
}
