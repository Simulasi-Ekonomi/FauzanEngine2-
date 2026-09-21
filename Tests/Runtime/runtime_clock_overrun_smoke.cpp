#include "Runtime/RuntimeClock.h"
#include <cstdio>
int main() {
    using namespace NeoEngine;
    RuntimeClock preInit;
    if (preInit.Advance(0.01F)) return 10;
    if (preInit.LastError() != RuntimeClockError::NotInitialized) return 11;
    RuntimeClock invalid;
    if (invalid.Initialize({0.0F, 0.25F, 8})) return 12;
    if (invalid.LastError() != RuntimeClockError::InvalidConfiguration) return 13;
    RuntimeClock invalidMax;
    if (invalidMax.Initialize({0.01F, 0.01F, 0})) return 14;
    RuntimeClock clock;
    if (!clock.Initialize({0.01F, 10.0F, 2})) return 1;
    if (!clock.Advance(0.051F)) return 2;
    if (clock.Snapshot().pendingFixedSteps != 2U) return 3;
    if (clock.Snapshot().droppedFixedStepCount == 0U || clock.Snapshot().droppedFixedSeconds <= 0.0F) return 4;
    if (clock.LastError() != RuntimeClockError::FixedStepOverrun) return 5;
    if (clock.Snapshot().frameCount != 1U) return 15;
    if (clock.Snapshot().fixedStepCount > 2U) return 16;
    if (clock.Snapshot().pendingFixedSteps > 2U) return 17;
    if (clock.Snapshot().droppedFixedStepCount < 1U) return 18;
    if (clock.Snapshot().droppedFixedSeconds <= 0.0F) return 19;
    if (clock.SetTimeScale(0.0F)) return 20;
    if (clock.LastError() != RuntimeClockError::InvalidScale) return 21;
    if (!clock.SetTimeScale(1.0F)) return 22;
    if (!clock.SetPaused(true)) return 23;
    if (clock.Snapshot().paused != true) return 24;
    if (!clock.SetPaused(false)) return 25;
    std::printf("RUNTIME_CLOCK_OVERRUN_SMOKE_OK bounded=1 accounted=1\\n");
    return 0;
}
