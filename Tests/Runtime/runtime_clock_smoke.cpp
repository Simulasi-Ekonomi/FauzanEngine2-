#include "Runtime/RuntimeClock.h"

#include <cmath>
#include <cstdio>
#include <limits>

int main() {
    using namespace NeoEngine;
    RuntimeClock clock; if (!clock.Initialize({1.0F / 60.0F, 0.25F, 4}) || !clock.Advance(1.0F / 30.0F)) return 1;
    if (clock.Snapshot().pendingFixedSteps != 2 || !clock.ConsumeFixedStep() || !clock.ConsumeFixedStep() || clock.ConsumeFixedStep()) return 1;
    if (!clock.SetTimeScale(0.5F) || !clock.Advance(1.0F / 30.0F) || std::fabs(clock.Snapshot().scaledDeltaSeconds - 1.0F / 60.0F) > 0.0001F) return 1;
    if (!clock.SetPaused(true) || !clock.Advance(0.1F) || clock.Snapshot().scaledDeltaSeconds != 0.0F || clock.Snapshot().pendingFixedSteps != 0) return 1;
    if (clock.SetTimeScale(5.0F) || clock.LastError() != RuntimeClockError::InvalidScale || clock.Advance(-0.1F) || clock.LastError() != RuntimeClockError::InvalidDelta) return 1;
    RuntimeClock boundary;
    if (!boundary.Initialize({1.0F, 2.0F, 4}) || !boundary.Advance(0.9999995F) || boundary.Snapshot().pendingFixedSteps != 0) return 1;
    if (!boundary.Advance(0.0000005F) || boundary.Snapshot().pendingFixedSteps != 1) return 1;
    RuntimeClock invalid;
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    if (invalid.Initialize({nan, 0.25F, 4}) || invalid.LastError() != RuntimeClockError::InvalidConfiguration) return 1;
    if (invalid.Initialize({1.0F, inf, 4}) || invalid.LastError() != RuntimeClockError::InvalidConfiguration) return 1;
    std::printf("RUNTIME_CLOCK_SMOKE_OK fixed=2 pause=1 scale=0.5 clamp=0.25 boundary=exact config-finite=1\n");
    return 0;
}
