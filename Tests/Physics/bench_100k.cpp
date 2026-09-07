#include "Core/ECS/ArchetypeManager.h"
#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Threading/JobSystem.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

#if defined(__linux__)
#include <sys/resource.h>
#endif

using namespace NeoEngine;

int main(int argc, char** argv) {
    constexpr int kEntityCount = 100000;
    constexpr int kColumns = 1000;
    constexpr float kSpacingX = 0.50f;
    constexpr float kSpacingZ = 0.4330127f;
    constexpr float kRadius = 0.251f;
    constexpr size_t kMinimumCollisions = 200000;
    constexpr double kTargetMs = 5.0;
    constexpr int kWarmupFrames = 3;
    constexpr int kMeasuredFrames = 10;
    constexpr int kWorkerCount = 8;

    const int frameCount = argc > 1 ? std::max(1, std::atoi(argv[1])) : kMeasuredFrames;
    const int workerCount = argc > 2 ? std::max(1, std::atoi(argv[2])) : kWorkerCount;
    const bool phaseTimingEnabled = argc > 3 ? std::atoi(argv[3]) != 0 : true;

    std::printf("=== FAUZANENGINE XPBD 100K / 200K COLLISION PERFORMANCE GATE ===\n");
    std::printf("Bodies: %d | Required actual contacts: >= %zu | Radius: %.3f | Hex spacing: %.6f / %.6f\n",
                kEntityCount, kMinimumCollisions, kRadius, kSpacingX, kSpacingZ);
    std::printf("Workers: %d | Warmup: %d | Measured frames: %d | Target: < %.3f ms/frame\n",
                workerCount, kWarmupFrames, frameCount, kTargetMs);

    JobSystem::Get().Initialize(workerCount);
    ArchetypeManager entities;
    auto physics = std::make_unique<XPBDPhysicsSystem>();
    physics->SetTimingEnabled(phaseTimingEnabled);
    physics->SetProbeMetricsEnabled(phaseTimingEnabled);
    const uint32_t flags = COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER;
    std::vector<EntityID> ids;
    ids.reserve(kEntityCount);

    const auto setCollisionWorkload = [&entities, &ids]() {
        for (int index = 0; index < kEntityCount; ++index) {
            const EntityID id = ids[static_cast<size_t>(index)];
            const int row = index / kColumns;
            const int column = index % kColumns;
            const float stagger = (row & 1) ? 0.5f : 0.0f;
            entities.SetPosX(id, (static_cast<float>(column) + stagger) * kSpacingX);
            entities.SetPosZ(id, static_cast<float>(row) * kSpacingZ);
            entities.SetVelX(id, 0.0f);
            entities.SetVelZ(id, 0.0f);
            entities.SetRadius(id, kRadius);
            entities.SetInvMass(id, 1.0f);
        }
    };

    for (int index = 0; index < kEntityCount; ++index)
        ids.push_back(entities.CreateEntity(flags));
    setCollisionWorkload();

    for (int frame = 0; frame < kWarmupFrames; ++frame)
        physics->Step(entities, 1.0f / 60.0f);

    std::vector<double> times;
    times.reserve(static_cast<size_t>(frameCount));
    size_t totalContacts = 0;
    size_t totalCandidatePairs = 0;
    StepTimingStats timingTotals{};
    BroadphaseTimingStats broadphaseTotals{};

    for (int frame = 0; frame < frameCount; ++frame) {
        // Reset outside the timer so every measured Step receives the same
        // full 100k-body collision workload.
        setCollisionWorkload();
        const auto started = std::chrono::steady_clock::now();
        physics->Step(entities, 1.0f / 60.0f);
        const auto finished = std::chrono::steady_clock::now();
        times.push_back(std::chrono::duration<double, std::milli>(finished - started).count());

        totalContacts += physics->GetManifoldCount();
        totalCandidatePairs += physics->GetBroadphaseStats().candidatePairs;

        const StepTimingStats& timing = physics->GetStepTimingStats();
        timingTotals.buildFlatMs += timing.buildFlatMs;
        timingTotals.setupMs += timing.setupMs;
        timingTotals.broadphaseMs += timing.broadphaseMs;
        timingTotals.integrateMs += timing.integrateMs;
        timingTotals.islandsAndGraphMs += timing.islandsAndGraphMs;
        timingTotals.solveMs += timing.solveMs;
        timingTotals.mergeMs += timing.mergeMs;
        timingTotals.writeBackMs += timing.writeBackMs;

        const BroadphaseTimingStats& bt = physics->GetBroadphaseTimingStats();
        broadphaseTotals.boundsMs += bt.boundsMs;
        broadphaseTotals.gridBuildMs += bt.gridBuildMs;
        broadphaseTotals.pairTraversalMs += bt.pairTraversalMs;
        broadphaseTotals.gatherMs += bt.gatherMs;
        broadphaseTotals.emitMs += bt.emitMs;
    }

    std::sort(times.begin(), times.end());
    double totalMs = 0.0;
    for (double value : times) totalMs += value;
    const double averageMs = totalMs / static_cast<double>(times.size());
    const size_t p50Index = (times.size() - 1) / 2;
    const size_t p95Index = static_cast<size_t>(std::ceil((times.size() - 1) * 0.95));
    const size_t p99Index = static_cast<size_t>(std::ceil((times.size() - 1) * 0.99));
    const double avgContacts = static_cast<double>(totalContacts) / static_cast<double>(frameCount);
    const double avgCandidates = static_cast<double>(totalCandidatePairs) / static_cast<double>(frameCount);
    const double p95Ms = times[p95Index];

    std::printf("Contacts/frame: %.1f | Collision candidates/frame: %.1f\n", avgContacts, avgCandidates);
    std::printf("Frame time ms: avg=%.3f p50=%.3f p95=%.3f p99=%.3f min=%.3f max=%.3f\n",
                averageMs, times[p50Index], p95Ms, times[p99Index], times.front(), times.back());
    if (phaseTimingEnabled) {
        std::printf("XPBD phase ms/frame: flat=%.3f setup=%.3f broadphase=%.3f integrate=%.3f islands_graph=%.3f solve=%.3f merge=%.3f writeback=%.3f\n",
                    timingTotals.buildFlatMs / frameCount, timingTotals.setupMs / frameCount,
                    timingTotals.broadphaseMs / frameCount, timingTotals.integrateMs / frameCount,
                    timingTotals.islandsAndGraphMs / frameCount, timingTotals.solveMs / frameCount,
                    timingTotals.mergeMs / frameCount, timingTotals.writeBackMs / frameCount);
        std::printf("Grid phase ms/frame: bounds=%.3f build=%.3f pair_traversal=%.3f gather=%.3f emit=%.3f\n",
                    broadphaseTotals.boundsMs / frameCount, broadphaseTotals.gridBuildMs / frameCount,
                    broadphaseTotals.pairTraversalMs / frameCount, broadphaseTotals.gatherMs / frameCount,
                    broadphaseTotals.emitMs / frameCount);
    }
#if defined(__linux__)
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) == 0)
        std::printf("Peak resident memory: %.2f MiB\n", static_cast<double>(usage.ru_maxrss) / 1024.0);
#endif

    const bool collisionGate = avgContacts >= static_cast<double>(kMinimumCollisions);
    const bool performanceGate = p95Ms < kTargetMs;
    if (!collisionGate)
        std::fprintf(stderr, "FAIL: workload produced fewer than %zu actual contacts/frame.\n", kMinimumCollisions);
    if (!performanceGate)
        std::fprintf(stderr, "FAIL: p95 frame time %.3f ms is not below %.3f ms.\n", p95Ms, kTargetMs);

    JobSystem::Get().Shutdown();
    if (!collisionGate || !performanceGate) return 1;
    std::printf("XPBD_100K_200K_SUB5MS_GATE_OK\n");
    return 0;
}
