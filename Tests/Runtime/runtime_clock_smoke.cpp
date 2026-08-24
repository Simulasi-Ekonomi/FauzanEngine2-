#include "Runtime/RuntimeClock.h"

#include <cmath>
#include <cstdio>

int main() {
    using namespace NeoEngine;
    RuntimeClock clock; if (!clock.Initialize({1.0F / 60.0F, 0.25F, 4}) || !clock.Advance(1.0F / 30.0F)) return 1;
    if (clock.Snapshot().pendingFixedSteps != 2 || !clock.ConsumeFixedStep() || !clock.ConsumeFixedStep() || clock.ConsumeFixedStep()) return 1;
    if (!clock.SetTimeScale(0.5F) || !clock.Advance(1.0F / 30.0F) || std::fabs(clock.Snapshot().scaledDeltaSeconds - 1.0F / 60.0F) > 0.0001F) return 1;
    if (!clock.SetPaused(true) || !clock.Advance(0.1F) || clock.Snapshot().scaledDeltaSeconds != 0.0F || clock.Snapshot().pendingFixedSteps != 0) return 1;
    if (clock.SetTimeScale(5.0F) || clock.LastError() != RuntimeClockError::InvalidScale || clock.Advance(-0.1F) || clock.LastError() != RuntimeClockError::InvalidDelta) return 1;
    std::printf("RUNTIME_CLOCK_SMOKE_OK fixed=2 pause=1 scale=0.5 clamp=0.25\n");
    return 0;
}
