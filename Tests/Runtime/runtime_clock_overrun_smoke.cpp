#include "Runtime/RuntimeClock.h"
#include <cstdio>
int main() {
    using namespace NeoEngine;
    RuntimeClock clock;
    if (!clock.Initialize({0.01F, 10.0F, 2})) return 1;
    if (!clock.Advance(0.051F)) return 2;
    if (clock.Snapshot().pendingFixedSteps != 2U) return 3;
    if (clock.Snapshot().droppedFixedStepCount == 0U || clock.Snapshot().droppedFixedSeconds <= 0.0F) return 4;
    if (clock.LastError() != RuntimeClockError::FixedStepOverrun) return 5;
    std::printf("RUNTIME_CLOCK_OVERRUN_SMOKE_OK bounded=1 accounted=1
");
    return 0;
}
