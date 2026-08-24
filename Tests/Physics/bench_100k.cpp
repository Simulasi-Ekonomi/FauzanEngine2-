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
    constexpr int kFrames = 10;
    constexpr int kColumns = 1000;
    constexpr int kWorkerCount = 8;
    constexpr int kContactSolverIterations = 3;
    const int awakeStride = argc > 1 ? std::max(1, std::atoi(argv[1])) : 10;
    const float spacing = argc > 2 ? std::max(0.01f, static_cast<float>(std::atof(argv[2]))) : 1.0f;
    const int frameCount = argc > 3 ? std::max(1, std::atoi(argv[3])) : kFrames;
    const int workerCount = argc > 4 ? std::max(1, std::atoi(argv[4])) : kWorkerCount;
    const bool phaseTimingEnabled = argc > 5 ? std::atoi(argv[5]) != 0 : true;

    std::printf("=== FAUZANENGINE XPBD 100K ENTITY BASELINE ===\n");
    std::printf("Entities: %d | Awake: %d (%.1f%%) | Radius: 0.20 | Spacing: %.2f\n",
                kEntityCount, kEntityCount / awakeStride, 100.0f / awakeStride, spacing);
    std::printf("Workers: %d | Contact solver iterations: %d | Fixed step input: %.6f s\n",
                workerCount, kContactSolverIterations, 1.0f / 60.0f);
    std::printf("Phase timing instrumentation: %s\n", phaseTimingEnabled ? "enabled" : "disabled");

    JobSystem::Get().Initialize(workerCount);
    ArchetypeManager entities;
    auto physics = std::make_unique<XPBDPhysicsSystem>();
    physics->SetTimingEnabled(phaseTimingEnabled);
    physics->SetProbeMetricsEnabled(phaseTimingEnabled);
    const uint32_t flags = COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER;

    for (int index = 0; index < kEntityCount; ++index) {
        const EntityID id = entities.CreateEntity(flags);
        entities.SetPosX(id, static_cast<float>(index % kColumns) * spacing);
        entities.SetPosZ(id, static_cast<float>(index / kColumns) * spacing);
        entities.SetRadius(id, 0.20f);
        entities.SetInvMass(id, 1.0f);
        if (index % awakeStride == 0) entities.SetVelX(id, 0.10f);
    }

    for (int frame = 0; frame < 3; ++frame) physics->Step(entities, 1.0f / 60.0f);

    std::vector<double> times;
    times.reserve(frameCount);
    size_t contacts = 0;
    size_t totalColorCount = 0;
    size_t totalMaxColorBatch = 0;
    size_t totalManifoldCacheSize = 0;
    size_t totalCandidatePairs = 0;
    size_t totalOccupiedCells = 0;
    size_t totalActiveCells = 0;
    StepTimingStats timingTotals{};
    BroadphaseTimingStats broadphaseTimingTotals{};
    ManifoldCacheStats manifoldCacheTotals{};
    for (int frame = 0; frame < frameCount; ++frame) {
        const auto started = std::chrono::steady_clock::now();
        physics->Step(entities, 1.0f / 60.0f);
        const auto finished = std::chrono::steady_clock::now();
        times.push_back(std::chrono::duration<double, std::milli>(finished - started).count());
        contacts += physics->GetManifoldCount();
        totalManifoldCacheSize += physics->GetManifoldCacheSize();
        totalColorCount += physics->GetContactColorCount();
        totalMaxColorBatch += physics->GetMaxContactColorBatch();
        const BroadphaseStats& broadphaseStats = physics->GetBroadphaseStats();
        totalCandidatePairs += broadphaseStats.candidatePairs;
        totalOccupiedCells += broadphaseStats.occupiedCells;
        totalActiveCells += broadphaseStats.activeCells;
        const BroadphaseTimingStats& broadphaseTiming = physics->GetBroadphaseTimingStats();
        broadphaseTimingTotals.boundsMs += broadphaseTiming.boundsMs;
        broadphaseTimingTotals.gridBuildMs += broadphaseTiming.gridBuildMs;
        broadphaseTimingTotals.pairTraversalMs += broadphaseTiming.pairTraversalMs;
        broadphaseTimingTotals.gatherMs += broadphaseTiming.gatherMs;
        broadphaseTimingTotals.emitMs += broadphaseTiming.emitMs;
        const StepTimingStats& timing = physics->GetStepTimingStats();
        timingTotals.buildFlatMs += timing.buildFlatMs;
        timingTotals.setupMs += timing.setupMs;
        timingTotals.broadphaseMs += timing.broadphaseMs;
        timingTotals.integrateMs += timing.integrateMs;
        timingTotals.islandsAndGraphMs += timing.islandsAndGraphMs;
        timingTotals.solveMs += timing.solveMs;
        timingTotals.mergeMs += timing.mergeMs;
        timingTotals.writeBackMs += timing.writeBackMs;
        const ManifoldCacheStats& frameCache = physics->GetManifoldCacheStats();
        manifoldCacheTotals.hits += frameCache.hits;
        manifoldCacheTotals.createAttempts += frameCache.createAttempts;
        manifoldCacheTotals.createSuccesses += frameCache.createSuccesses;
        manifoldCacheTotals.rejections += frameCache.rejections;
        manifoldCacheTotals.probeSteps += frameCache.probeSteps;
        manifoldCacheTotals.maxProbeDepth = std::max(manifoldCacheTotals.maxProbeDepth, frameCache.maxProbeDepth);
    }
    std::sort(times.begin(), times.end());
    double total = 0.0;
    for (double value : times) total += value;
    const double average = total / static_cast<double>(times.size());
    const size_t p50Index = (times.size() - 1) / 2;
    const size_t p95Index = static_cast<size_t>(std::ceil((times.size() - 1) * 0.95));
    const size_t p99Index = static_cast<size_t>(std::ceil((times.size() - 1) * 0.99));

    std::printf("Frames: %d | Avg contacts: %.1f\n", frameCount, static_cast<double>(contacts) / frameCount);
    std::printf("Contact graph colors/frame: %.1f\n", static_cast<double>(totalColorCount) / frameCount);
    std::printf("Largest contact color batch/frame: %.1f\n", static_cast<double>(totalMaxColorBatch) / frameCount);
    std::printf("Grid/frame: candidates=%.1f occupied_cells=%.1f active_cells=%.1f\n",
                static_cast<double>(totalCandidatePairs) / frameCount,
                static_cast<double>(totalOccupiedCells) / frameCount,
                static_cast<double>(totalActiveCells) / frameCount);
    if (phaseTimingEnabled) {
        std::printf("XPBD phase ms/frame: flat=%.3f setup=%.3f broadphase=%.3f integrate=%.3f islands_graph=%.3f solve=%.3f merge=%.3f writeback=%.3f\n",
                    timingTotals.buildFlatMs / frameCount, timingTotals.setupMs / frameCount,
                    timingTotals.broadphaseMs / frameCount, timingTotals.integrateMs / frameCount,
                    timingTotals.islandsAndGraphMs / frameCount, timingTotals.solveMs / frameCount,
                    timingTotals.mergeMs / frameCount, timingTotals.writeBackMs / frameCount);
        std::printf("Grid phase ms/frame: bounds=%.3f build=%.3f pair_traversal=%.3f gather=%.3f emit=%.3f\n",
                    broadphaseTimingTotals.boundsMs / frameCount,
                    broadphaseTimingTotals.gridBuildMs / frameCount,
                    broadphaseTimingTotals.pairTraversalMs / frameCount,
                    broadphaseTimingTotals.gatherMs / frameCount,
                    broadphaseTimingTotals.emitMs / frameCount);
    }
    if (!phaseTimingEnabled) {
        std::printf("Manifold cache/frame: profiling disabled in headline mode\n");
    } else {
        const double cacheLookups = static_cast<double>(manifoldCacheTotals.hits + manifoldCacheTotals.createAttempts);
        std::printf("Manifold cache/frame: entries=%.1f hits=%.1f create_attempts=%.1f create_successes=%.1f rejections=%.1f",
                    static_cast<double>(totalManifoldCacheSize) / frameCount,
                    static_cast<double>(manifoldCacheTotals.hits) / frameCount,
                    static_cast<double>(manifoldCacheTotals.createAttempts) / frameCount,
                    static_cast<double>(manifoldCacheTotals.createSuccesses) / frameCount,
                    static_cast<double>(manifoldCacheTotals.rejections) / frameCount);
        std::printf(" probes/lookup=%.3f max_probe=%zu",
                    cacheLookups > 0.0 ? static_cast<double>(manifoldCacheTotals.probeSteps) / cacheLookups : 0.0,
                    manifoldCacheTotals.maxProbeDepth);
        std::printf("\n");
    }
    std::printf("Frame time ms: avg=%.3f p50=%.3f p95=%.3f p99=%.3f min=%.3f max=%.3f\n",
                average, times[p50Index], times[p95Index], times[p99Index], times.front(), times.back());
#if defined(__linux__)
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) == 0)
        std::printf("Peak resident memory: %.2f MiB\n", static_cast<double>(usage.ru_maxrss) / 1024.0);
#endif
    std::printf("XPBD_100K_BASELINE_COMPLETE\n");

    JobSystem::Get().Shutdown();
    return 0;
}
