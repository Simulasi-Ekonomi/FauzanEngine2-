#include "RuntimeVerticalSliceGate.h"
#include <limits>
#include <cstdint>
namespace NeoEngine {
bool RuntimeVerticalSliceGate::Validate(NeoRuntime& runtime, bool executeTick, VerticalSliceGateReceipt& receipt) {
    receipt = {};
    if (runtime.State() != RuntimeState::Initialized) {
        receipt.error = VerticalSliceGateError::RuntimeNotInitialized;
        return false;
    }
    receipt.initialized = true;

    const auto& sceneEcs = runtime.SceneECS();
    const uint64_t initialRevision = sceneEcs.revision;
    if (initialRevision == 0U || initialRevision == std::numeric_limits<uint64_t>::max()) {
        receipt.error = VerticalSliceGateError::SceneECSRevisionMismatch;
        return false;
    }
    receipt.sceneValid = runtime.Scene() != nullptr;
    if (!receipt.sceneValid) {
        receipt.error = VerticalSliceGateError::SceneMissing;
        return false;
    }
    receipt.ecsValid = runtime.ECS() != nullptr;
    if (!receipt.ecsValid) {
        receipt.error = VerticalSliceGateError::ECSMissing;
        return false;
    }

    const uint64_t sceneEntities = runtime.Scene()->AliveCount();
    if (sceneEntities == std::numeric_limits<uint64_t>::max() ||
        sceneEntities > std::numeric_limits<uint32_t>::max() ||
        sceneEcs.sceneCount > std::numeric_limits<uint32_t>::max() ||
        sceneEcs.ecsCount > std::numeric_limits<uint32_t>::max()) {
        receipt.error = VerticalSliceGateError::SceneECSMismatch;
        return false;
    }
    receipt.sceneEntities = static_cast<uint32_t>(sceneEntities);
    receipt.ecsEntities = static_cast<uint32_t>(sceneEcs.ecsCount);
    receipt.sceneECSConsistent = sceneEcs.sceneCount == sceneEntities && sceneEcs.ecsCount == sceneEntities;
    if (receipt.sceneEntities != receipt.ecsEntities) {
        receipt.error = VerticalSliceGateError::SceneECSMismatch;
        return false;
    }
    if (!receipt.sceneECSConsistent) {
        receipt.error = VerticalSliceGateError::SceneECSMismatch;
        return false;
    }

    const uint64_t physicsRevision = runtime.ECS()->GetPhysicsRevision();
    if (physicsRevision == std::numeric_limits<uint64_t>::max()) {
        receipt.error = VerticalSliceGateError::SceneECSRevisionMismatch;
        return false;
    }
    receipt.sceneECSRevisionValid =
        physicsRevision != 0U &&
        physicsRevision != std::numeric_limits<uint64_t>::max() &&
        physicsRevision == sceneEcs.revision;
    if (physicsRevision < initialRevision) {
        receipt.error = VerticalSliceGateError::SceneECSRevisionMismatch;
        return false;
    }
    if (!receipt.sceneECSRevisionValid) {
        receipt.error = VerticalSliceGateError::SceneECSRevisionMismatch;
        return false;
    }

    receipt.sceneMeshRegistryValid = runtime.SceneMeshes() != nullptr;
    if (!receipt.sceneMeshRegistryValid) {
        receipt.error = VerticalSliceGateError::SceneMeshMissing;
        return false;
    }
    receipt.assetsValid = runtime.Assets() != nullptr;
    if (!receipt.assetsValid) {
        receipt.error = VerticalSliceGateError::AssetsMissing;
        return false;
    }
    receipt.resourcesValid = runtime.Resources() != nullptr;
    if (!receipt.resourcesValid) {
        receipt.error = VerticalSliceGateError::ResourcesMissing;
        return false;
    }
    receipt.replicationValid = runtime.Replication() != nullptr;
    if (runtime.SceneMeshes()->Size() > std::numeric_limits<uint32_t>::max()) {
        receipt.error = VerticalSliceGateError::SceneMeshMissing;
        return false;
    }
    if (!receipt.replicationValid) {
        receipt.error = VerticalSliceGateError::ReplicationMissing;
        return false;
    }

    if (!executeTick) {
        receipt.error = VerticalSliceGateError::None;
        return true;
    }

    const uint32_t preSceneEntities = receipt.sceneEntities;
    const uint64_t prePhysicsRevision = physicsRevision;
    if (preSceneEntities != sceneEcs.sceneCount || preSceneEntities != sceneEcs.ecsCount || prePhysicsRevision != initialRevision ||
        sceneEcs.revision == std::numeric_limits<uint64_t>::max()) {
        receipt.error = VerticalSliceGateError::SceneECSMismatch;
        return false;
    }
    if (!runtime.Tick()) {
        receipt.error = VerticalSliceGateError::TickRejected;
        return false;
    }
    receipt.tickAccepted = true;
    if (runtime.State() != RuntimeState::Initialized) {
        receipt.error = VerticalSliceGateError::TickRejected;
        return false;
    }

    if (runtime.State() != RuntimeState::Initialized || runtime.Scene() == nullptr || runtime.ECS() == nullptr ||
        runtime.SceneMeshes() == nullptr || runtime.Assets() == nullptr || runtime.Resources() == nullptr ||
        runtime.Replication() == nullptr) {
        receipt.error = VerticalSliceGateError::SceneECSMismatch;
        return false;
    }
    const uint64_t postSceneEntities = runtime.Scene()->AliveCount();
    const uint64_t postPhysicsRevision = runtime.ECS()->GetPhysicsRevision();
    if (postSceneEntities == std::numeric_limits<uint64_t>::max() || postPhysicsRevision == std::numeric_limits<uint64_t>::max()) {
        receipt.error = VerticalSliceGateError::SceneECSMismatch;
        return false;
    }
    if (postSceneEntities > std::numeric_limits<uint32_t>::max() || runtime.SceneECS().sceneCount > std::numeric_limits<uint32_t>::max() || runtime.SceneECS().ecsCount > std::numeric_limits<uint32_t>::max() ||
        postSceneEntities != runtime.SceneECS().sceneCount ||
        postSceneEntities != runtime.SceneECS().ecsCount || runtime.SceneECS().sceneCount != runtime.SceneECS().ecsCount ||
        runtime.SceneECS().revision < prePhysicsRevision) {
        receipt.error = VerticalSliceGateError::SceneECSMismatch;
        return false;
    }
    if (runtime.SceneECS().revision < prePhysicsRevision || runtime.SceneECS().revision == 0U ||
        runtime.SceneECS().revision == std::numeric_limits<uint64_t>::max() ||
        postPhysicsRevision != runtime.SceneECS().revision) {
        receipt.error = VerticalSliceGateError::SceneECSRevisionMismatch;
        return false;
    }
    receipt.sceneEntities = static_cast<uint32_t>(postSceneEntities);
    receipt.ecsEntities = static_cast<uint32_t>(runtime.SceneECS().ecsCount);
    receipt.sceneECSRevisionValid = runtime.SceneECS().revision != 0U &&
        runtime.SceneECS().revision != std::numeric_limits<uint64_t>::max() &&
        postPhysicsRevision == runtime.SceneECS().revision;
    if (receipt.sceneEntities != receipt.ecsEntities || !receipt.sceneECSRevisionValid) {
        receipt.error = VerticalSliceGateError::SceneECSMismatch;
        return false;
    }
    if (receipt.sceneEntities < preSceneEntities && postPhysicsRevision == prePhysicsRevision) {
        receipt.error = VerticalSliceGateError::SceneECSMismatch;
        return false;
    }
    receipt.error = VerticalSliceGateError::None;
    return true;
}

}
