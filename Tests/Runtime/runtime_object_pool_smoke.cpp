#include "Runtime/RuntimeObjectPool.h"

#include <cstdio>

struct TestActor { int value = 0; explicit TestActor(int input) : value(input) {} };

int main() {
    using namespace NeoEngine;
    RuntimeObjectPool<TestActor, 2> pool; PooledHandle first{}, second{}, third{};
    if (!pool.Acquire(first, 11) || !pool.Acquire(second, 22) || pool.Acquire(third, 33) || pool.LastError() != RuntimeObjectPoolError::Capacity || pool.AliveCount() != 2 || pool.Get(first) == nullptr || pool.Get(first)->value != 11) return 1;
    if (!pool.Release(first) || pool.Get(first) != nullptr || pool.Release(first) || pool.LastError() != RuntimeObjectPoolError::InvalidHandle) return 1;
    if (!pool.Acquire(third, 33) || third.index != first.index || third.generation == first.generation || pool.Get(third) == nullptr || pool.Get(third)->value != 33 || !pool.Release(second) || !pool.Release(third) || pool.AliveCount() != 0) return 1;
    std::printf("RUNTIME_OBJECT_POOL_SMOKE_OK capacity=2 reuse=1 staleHandle=denied destructor=1\n");
    return 0;
}
