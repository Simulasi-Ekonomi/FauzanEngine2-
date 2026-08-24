#include "Runtime/RuntimeTimerQueue.h"

#include <cstdio>
#include <vector>

int main() {
    using namespace NeoEngine; RuntimeTimerQueue queue; RuntimeTimerHandle once{}, repeat{}; std::vector<RuntimeTimerFire> fires;
    if (!queue.Schedule(0.10F, false, 10, once) || !queue.Schedule(0.05F, true, 20, repeat) || !queue.Advance(0.05F, fires) || fires.size() != 1 || fires[0].userTag != 20 || !queue.Advance(0.05F, fires) || fires.size() != 2 || fires[0].userTag != 10 || fires[1].userTag != 20 || queue.ActiveCount() != 1 || !queue.Cancel(repeat) || queue.Cancel(repeat) || queue.LastError() != RuntimeTimerError::InvalidHandle || queue.Advance(-1.0F, fires) || queue.LastError() != RuntimeTimerError::InvalidDuration) return 1;
    std::printf("RUNTIME_TIMER_QUEUE_SMOKE_OK oneShot=1 repeat=2 cancel=1 deterministic=1\n"); return 0;
}
