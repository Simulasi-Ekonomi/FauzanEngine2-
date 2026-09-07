#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Core/ECS/ArchetypeManager.h"
#include "Threading/JobSystem.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

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
    std::printf("Bodies: %d | Required collisions: >= %zu | Radius: %.3f | Hex spacing: %.6f / %.6f\n",
                kEntityCount, kMinimumCollisions, kRadius, kSpacingX, kSpacingZ);
    std::printf("Workers: %d | Warmup: %d | Measured frames: %d | Target: < %.3f ms/frame\n",
                workerCount, kWarmupFrames, frameCount, kTargetMs);

    JobSystem::Get().Initialize(workerCount);
    ArchetypeManager entities;
    auto physics = std::make_unique<XPBDPhysicsSystem>();
    physics->SetTimingEnabled(phaseTimingEnabled);
    physics->SetProbeMetricsEnabled(phaseTimingEnabled);
    const uint32_t flags = COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER;

    for (int index = 0; index < kEntityCount; ++index) {
        const EntityID id = entities.CreateEntity(flags);
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

    for (int frame = 0; frame < kWarmupFrames; ++frame)
        physics->Step(entities, 1.0f / 60.0f);

    std::vector<double> times;
    times.reserve(static_cast<size_t>(frameCount));
    size_t totalContacts = 0;
    size_t totalCandidatePairs = 0;
    for (int frame = 0; frame < frameCount; ++frame) {
        const auto started = std::chrono::steady_clock::now();
        physics->Step(entities, 1.0f / 60.0f);
        const auto finished = std::chrono::steady_clock::now();
        times.push_back(std::chrono::duration<double, std::milli>(finished - started).count());
        totalContacts += physics->GetManifoldCount();
        totalCandidatePairs += physics->GetBroadphaseStats().candidatePairs;
    }

    std::sort(times.begin(), times.end());
    double totalMs = 0.0;
    for (double value : times) totalMs += value;
    const double averageMs = totalMs / static_cast<double>(times.size());
    const size_t p50Index = (times.size() - 1) / 2;
    const size_t p95Index = static_cast<size_t>(std::ceil((times.size() - 1) * 0.95));
    const double avgContacts = static_cast<double>(totalContacts) / frameCount;
    const double avgCandidates = static_cast<double>(totalCandidatePairs) / frameCount;
    const double p95Ms = times[p95Index];

    std::printf("Contacts/frame: %.1f | Collision candidates/frame: %.1f\n", avgContacts, avgCandidates);
    std::printf("Frame time ms: avg=%.3f p50=%.3f p95=%.3f min=%.3f max=%.3f\n",
                averageMs, times[p50Index], p95Ms, times.front(), times.back());

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
