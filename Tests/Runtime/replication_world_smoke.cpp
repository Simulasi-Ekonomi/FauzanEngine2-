#include "Runtime/ReplicationWorld.h"

#include <cmath>
#include <limits>
#include <memory>
#include <vector>

namespace {
bool RunStaleRemoteEntityRegression() {
    using namespace NeoEngine;
    auto scene = std::make_unique<SceneWorld>();
    auto client = std::make_unique<ReplicationWorld>(*scene, ReplicationRole::Client, 7U);
    SceneEntity entity{};
    if (!scene->Create(entity) || !scene->SetTransform(entity, {4.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}) || !client->RegisterEntity(entity, 501U, 8U) || !scene->Destroy(entity)) return false;
    ReplicationSnapshot snapshot{};
    snapshot.sequence = 1U; snapshot.serverTick = 1U; snapshot.count = 1U;
    snapshot.states[0] = {501U, 8U, 1U, {5.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}};
    std::vector<uint8_t> bytes; ReplicationError error = ReplicationError::None; ReplicationSnapshot decoded{};
    if (!ReplicationSnapshotCodec::Serialize(snapshot, bytes, error) || !ReplicationSnapshotCodec::Deserialize(bytes, decoded, error)) return false;
    ReplicationApplyReceipt receipt{91U, 92U, 93U, 94U, 95U, 96U, 97U, true};
    return !client->ApplyServerSnapshot(decoded, receipt) && client->LastError() == ReplicationError::InvalidEntity && receipt.sequence == 91U && receipt.serverTick == 92U && receipt.appliedEntities == 93U && receipt.spawnedEntities == 94U && receipt.despawnedEntities == 95U && receipt.interpolatedEntities == 96U && receipt.reconciledPredictions == 97U && receipt.accepted && client->SnapshotSequence() == 0U && client->RegisteredCount() == 1U;
}
}

int main() {
    using namespace NeoEngine;
    SceneWorld serverScene;
    SceneWorld clientScene;
    SceneEntity serverLocal{}, serverRemote{}, clientLocal{}, clientRemote{};
    if (!serverScene.Create(serverLocal) || !serverScene.Create(serverRemote) || !clientScene.Create(clientLocal) || !clientScene.Create(clientRemote)) return 1;
    if (!serverScene.SetTransform(serverLocal, {5.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}) || !serverScene.SetTransform(serverRemote, {20.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}) || !clientScene.SetTransform(clientLocal, {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}) || !clientScene.SetTransform(clientRemote, {10.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F})) return 2;

    ReplicationWorld server(serverScene, ReplicationRole::Server, 0U);
    ReplicationWorld client(clientScene, ReplicationRole::Client, 7U);
    ReplicationWorld invalidRole(serverScene, static_cast<ReplicationRole>(255U));
    if (invalidRole.RegisterEntity(serverLocal, 999U, 0U) || invalidRole.LastError() != ReplicationError::InvalidInput || invalidRole.RegisteredCount() != 0U || invalidRole.UnregisterEntity(999U) || invalidRole.LastError() != ReplicationError::InvalidInput) return 3;
    if (!server.RegisterEntity(serverLocal, 100U, 7U) || !server.RegisterEntity(serverRemote, 200U, 8U) || !client.RegisterEntity(clientLocal, 100U, 7U) || !client.RegisterEntity(clientRemote, 200U, 8U)) return 3;
    ReplicatedEntityState preservedAuthoritative{77U, 88U, 99U, {7.0F, 8.0F, 9.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}};
    if (client.AuthoritativeState(999U, preservedAuthoritative) || client.LastError() != ReplicationError::UnknownEntity || preservedAuthoritative.networkId != 77U || preservedAuthoritative.ownerId != 88U || preservedAuthoritative.stateRevision != 99U || preservedAuthoritative.transform.x != 7.0F) return 3;
    if (server.UnregisterEntity(0U) || server.LastError() != ReplicationError::InvalidNetworkId || server.RegisteredCount() != 2U) return 3;
    if (client.RegisterEntity(clientLocal, 200U, 7U) || client.LastError() != ReplicationError::DuplicateNetworkId) return 4;
    if (client.RegisterEntity(clientLocal, 201U, 7U) || client.LastError() != ReplicationError::DuplicateEntity || client.RegisteredCount() != 2U) return 4;
    if (server.RegisterEntity(serverLocal, 0U, 7U) || server.LastError() != ReplicationError::InvalidNetworkId || !serverScene.SetTransform(serverLocal, {6.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F})) return 5;
    SceneWorld staleScene;
    SceneEntity staleEntity{};
    if (!staleScene.Create(staleEntity) || !staleScene.SetTransform(staleEntity, {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F})) return 5;
    ReplicationWorld staleWorld(staleScene, ReplicationRole::Server);
    if (!staleWorld.RegisterEntity(staleEntity, 900U, 1U) || !staleScene.Destroy(staleEntity)) return 5;
    SceneEntity recycledEntity{};
    if (!staleScene.Create(recycledEntity) || recycledEntity.index != staleEntity.index || !staleScene.SetTransform(recycledEntity, {2.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F})) return 5;
    if (staleWorld.RegisterEntity(recycledEntity, 901U, 1U) || staleWorld.LastError() != ReplicationError::InvalidEntity || staleWorld.RegisteredCount() != 1U || !staleWorld.UnregisterEntity(900U) || !staleWorld.RegisterEntity(recycledEntity, 901U, 1U)) return 5;

    ReplicationSnapshot snapshot{};
    if (!server.BuildServerSnapshot(1U, snapshot) || snapshot.count != 2U || snapshot.sequence != 1U || snapshot.states[0].networkId != 100U || snapshot.states[1].networkId != 200U) return 6;
    std::vector<uint8_t> encoded;
    ReplicationError codecError = ReplicationError::None;
    if (!ReplicationSnapshotCodec::Serialize(snapshot, encoded, codecError) || encoded.empty() || codecError != ReplicationError::None) return 7;
    std::vector<uint8_t> preservedBytes{0xA5U};
    ReplicationSnapshot invalidCodecSnapshot = snapshot;
    invalidCodecSnapshot.states[0].transform.x = std::numeric_limits<float>::quiet_NaN();
    if (ReplicationSnapshotCodec::Serialize(invalidCodecSnapshot, preservedBytes, codecError) || codecError != ReplicationError::InvalidSnapshot || preservedBytes.size() != 1U || preservedBytes[0] != 0xA5U) return 7;
    ReplicationSnapshot invalidOrderSnapshot = snapshot;
    invalidOrderSnapshot.states[1].networkId = invalidOrderSnapshot.states[0].networkId;
    if (ReplicationSnapshotCodec::Serialize(invalidOrderSnapshot, preservedBytes, codecError) || codecError != ReplicationError::InvalidSnapshot || preservedBytes.size() != 1U || preservedBytes[0] != 0xA5U) return 7;
    ReplicationSnapshot decoded{};
    if (!ReplicationSnapshotCodec::Deserialize(encoded, decoded, codecError) || decoded.sequence != snapshot.sequence || decoded.states[1].transform.x != 20.0F) return 8;
    encoded.back() ^= 0x4FU;
    if (ReplicationSnapshotCodec::Deserialize(encoded, decoded, codecError) || codecError != ReplicationError::CorruptSnapshot) return 9;

    ReplicationPredictionReceipt prediction{};
    if (!client.PredictLocalInput(100U, 1.0F, 0.0F, prediction)) return 10;
    ReplicationApplyReceipt apply{};
    if (!client.ApplyServerSnapshot(snapshot, apply) || !apply.accepted || apply.appliedEntities != 2U || apply.reconciledPredictions != 1U || client.SnapshotSequence() != 1U) return 11;
    ReplicationAcknowledgement acknowledgement{};
    ReplicationAcknowledgement decodedAcknowledgement{};
    std::vector<uint8_t> acknowledgementBytes;
    if (!client.BuildClientAcknowledgement(acknowledgement) || !ReplicationAcknowledgementCodec::Serialize(acknowledgement, acknowledgementBytes, codecError) || !ReplicationAcknowledgementCodec::Deserialize(acknowledgementBytes, decodedAcknowledgement, codecError) || decodedAcknowledgement.sequence != 1U || decodedAcknowledgement.checksum != snapshot.checksum || !server.ApplyClientAcknowledgement(decodedAcknowledgement) || server.AcknowledgedSequence() != 1U) return 12;
    acknowledgementBytes.back() ^= 0x01U;
    if (ReplicationAcknowledgementCodec::Deserialize(acknowledgementBytes, decodedAcknowledgement, codecError) || codecError != ReplicationError::CorruptSnapshot || server.AcknowledgedSequence() != 1U) return 13;
    ReplicationAcknowledgement invalidAcknowledgement = acknowledgement;
    invalidAcknowledgement.sequence = 2U;
    if (server.ApplyClientAcknowledgement(invalidAcknowledgement) || server.LastError() != ReplicationError::InvalidAcknowledgement || server.AcknowledgedSequence() != 1U) return 13;
    const Transform3* localAfterReconcile = clientScene.GetTransform(clientLocal);
    const Transform3* remoteBeforeInterpolation = clientScene.GetTransform(clientRemote);
    if (localAfterReconcile == nullptr || std::abs(localAfterReconcile->x - 6.0F) > 0.0001F || remoteBeforeInterpolation == nullptr || std::abs(remoteBeforeInterpolation->x - 10.0F) > 0.0001F) return 14;
    ReplicationSnapshot chronologySnapshot = snapshot;
    chronologySnapshot.sequence = 4U;
    chronologySnapshot.serverTick = 0U;
    std::vector<uint8_t> chronologyBytes;
    if (!ReplicationSnapshotCodec::Serialize(chronologySnapshot, chronologyBytes, codecError) || !ReplicationSnapshotCodec::Deserialize(chronologyBytes, decoded, codecError) || client.ApplyServerSnapshot(decoded, apply) || client.LastError() != ReplicationError::StaleSnapshot || client.SnapshotSequence() != 1U) return 15;
    ReplicationSnapshot unknownEntitySnapshot = snapshot;
    unknownEntitySnapshot.sequence = 3U;
    unknownEntitySnapshot.states[0].transform.x = 123.0F;
    unknownEntitySnapshot.states[1].networkId = 999U;
    std::vector<uint8_t> unknownEntityBytes;
    ReplicationApplyReceipt preservedApply{91U, 92U, 93U, 94U, 95U, 96U, 97U, true};
    if (!ReplicationSnapshotCodec::Serialize(unknownEntitySnapshot, unknownEntityBytes, codecError) || !ReplicationSnapshotCodec::Deserialize(unknownEntityBytes, decoded, codecError) || client.ApplyServerSnapshot(decoded, preservedApply) || client.LastError() != ReplicationError::UnknownEntity || preservedApply.sequence != 91U || preservedApply.serverTick != 92U || preservedApply.appliedEntities != 93U || preservedApply.spawnedEntities != 94U || preservedApply.despawnedEntities != 95U || preservedApply.interpolatedEntities != 96U || preservedApply.reconciledPredictions != 97U || !preservedApply.accepted) return 16;
    localAfterReconcile = clientScene.GetTransform(clientLocal);
    if (localAfterReconcile == nullptr || std::abs(localAfterReconcile->x - 6.0F) > 0.0001F) return 17;
    if (!client.SetInterpolationAlphaPermille(500U) || !client.ApplyInterpolation(apply) || apply.interpolatedEntities != 1U) return 18;
    const Transform3* remoteAfterInterpolation = clientScene.GetTransform(clientRemote);
    if (remoteAfterInterpolation == nullptr || std::abs(remoteAfterInterpolation->x - 15.0F) > 0.0001F) return 19;
    SceneWorld interpolationScene;
    SceneEntity interpolationFirst{}, interpolationSecond{};
    if (!interpolationScene.Create(interpolationFirst) || !interpolationScene.Create(interpolationSecond) || !interpolationScene.SetTransform(interpolationFirst, {2.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}) || !interpolationScene.SetTransform(interpolationSecond, {3.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F})) return 19;
    ReplicationWorld interpolationClient(interpolationScene, ReplicationRole::Client, 7U);
    if (!interpolationClient.RegisterEntity(interpolationFirst, 401U, 8U) || !interpolationClient.RegisterEntity(interpolationSecond, 402U, 8U) || !interpolationClient.SetInterpolationAlphaPermille(500U) || !interpolationScene.Destroy(interpolationSecond)) return 19;
    ReplicationApplyReceipt preservedInterpolationReceipt{0U, 0U, 0U, 0U, 0U, 99U, 77U, false};
    if (interpolationClient.ApplyInterpolation(preservedInterpolationReceipt) || interpolationClient.LastError() != ReplicationError::SceneApplyRejected || preservedInterpolationReceipt.interpolatedEntities != 99U || preservedInterpolationReceipt.reconciledPredictions != 77U || preservedInterpolationReceipt.accepted) return 19;
    const Transform3* interpolationFirstAfterFailure = interpolationScene.GetTransform(interpolationFirst);
    if (interpolationFirstAfterFailure == nullptr || std::abs(interpolationFirstAfterFailure->x - 2.0F) > 0.0001F) return 19;
    SceneWorld extremeInterpolationScene;
    SceneEntity extremeEntity{};
    const float extreme = std::numeric_limits<float>::max();
    if (!extremeInterpolationScene.Create(extremeEntity) || !extremeInterpolationScene.SetTransform(extremeEntity, {extreme, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F})) return 19;
    ReplicationWorld extremeClient(extremeInterpolationScene, ReplicationRole::Client, 7U);
    if (!extremeClient.RegisterEntity(extremeEntity, 403U, 8U)) return 19;
    ReplicationSnapshot extremeSnapshot{};
    extremeSnapshot.sequence = 1U; extremeSnapshot.serverTick = 1U; extremeSnapshot.count = 1U;
    extremeSnapshot.states[0] = {403U, 8U, 1U, {-extreme, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}};
    std::vector<uint8_t> extremeBytes;
    if (!ReplicationSnapshotCodec::Serialize(extremeSnapshot, extremeBytes, codecError) || !ReplicationSnapshotCodec::Deserialize(extremeBytes, decoded, codecError) || !extremeClient.ApplyServerSnapshot(decoded, apply) || !extremeClient.SetInterpolationAlphaPermille(500U) || !extremeClient.ApplyInterpolation(apply) || apply.interpolatedEntities != 1U) return 19;
    const Transform3* extremeAfterInterpolation = extremeInterpolationScene.GetTransform(extremeEntity);
    if (extremeAfterInterpolation == nullptr || !std::isfinite(extremeAfterInterpolation->x) || std::abs(extremeAfterInterpolation->x) > 1.0F) return 19;
    if (client.ApplyServerSnapshot(snapshot, apply) || client.LastError() != ReplicationError::StaleSnapshot) return 20;
    ReplicationSnapshot perEntityStale = snapshot; perEntityStale.sequence = 2U; perEntityStale.states[0].stateRevision = 0U;
    std::vector<uint8_t> staleBytes;
    if (!ReplicationSnapshotCodec::Serialize(perEntityStale, staleBytes, codecError) || !ReplicationSnapshotCodec::Deserialize(staleBytes, decoded, codecError) || client.ApplyServerSnapshot(decoded, apply) || client.LastError() != ReplicationError::StaleSnapshot) return 21;
    if (client.PredictLocalInput(200U, 1.0F, 0.0F, prediction) || client.LastError() != ReplicationError::OwnershipRejected) return 22;
    if (!client.PredictLocalInput(100U, 0.5F, 0.0F, prediction)) return 21;
    ReplicationSnapshot ownershipTransfer = snapshot;
    ownershipTransfer.sequence = 2U;
    ownershipTransfer.serverTick = 2U;
    ownershipTransfer.states[0].ownerId = 8U;
    ownershipTransfer.states[0].transform.x = 8.0F;
    std::vector<uint8_t> ownershipTransferBytes;
    if (!ReplicationSnapshotCodec::Serialize(ownershipTransfer, ownershipTransferBytes, codecError) || !ReplicationSnapshotCodec::Deserialize(ownershipTransferBytes, decoded, codecError) || !client.ApplyServerSnapshot(decoded, apply) || apply.reconciledPredictions != 0U) return 22;
    ownershipTransfer.sequence = 3U;
    ownershipTransfer.serverTick = 3U;
    ownershipTransfer.states[0].ownerId = 7U;
    ownershipTransfer.states[0].transform.x = 9.0F;
    if (!ReplicationSnapshotCodec::Serialize(ownershipTransfer, ownershipTransferBytes, codecError) || !ReplicationSnapshotCodec::Deserialize(ownershipTransferBytes, decoded, codecError) || !client.ApplyServerSnapshot(decoded, apply) || apply.reconciledPredictions != 0U || !client.PredictLocalInput(100U, 0.25F, 0.0F, prediction)) return 23;
    if (client.SetInterpolationAlphaPermille(1001U) || client.LastError() != ReplicationError::InvalidInput) return 26;
    if (server.ApplyServerSnapshot(snapshot, apply) || server.LastError() != ReplicationError::NotClient || server.BuildServerSnapshot(2U, snapshot) == false) return 27;
    ReplicationAcknowledgement acknowledgement2{snapshot.sequence, snapshot.serverTick, snapshot.checksum};
    if (!server.ApplyClientAcknowledgement(acknowledgement2) || server.AcknowledgedSequence() != 2U) return 28;
    for (uint64_t tick = 3U; tick <= 66U; ++tick) if (!server.BuildServerSnapshot(tick, snapshot)) return 29;
    if (server.ApplyClientAcknowledgement(acknowledgement2) || server.LastError() != ReplicationError::StaleAcknowledgement || server.AcknowledgedSequence() != 2U || server.ApplyClientAcknowledgement(acknowledgement) || server.LastError() != ReplicationError::StaleAcknowledgement) return 30;
    ReplicationSnapshot preservedServerSnapshot = snapshot;
    if (!serverScene.Destroy(serverRemote) || server.BuildServerSnapshot(67U, preservedServerSnapshot) || server.LastError() != ReplicationError::InvalidEntity || preservedServerSnapshot.sequence != snapshot.sequence || preservedServerSnapshot.serverTick != snapshot.serverTick || preservedServerSnapshot.count != snapshot.count || preservedServerSnapshot.checksum != snapshot.checksum || server.SnapshotSequence() != 66U) return 31;
    if (!client.UnregisterEntity(200U) || client.IsRegistered(200U) || client.RegisteredCount() != 1U) return 29;

    SceneWorld dynamicScene;
    ReplicationWorld dynamicClient(dynamicScene, ReplicationRole::Client, 7U, true);
    ReplicationSnapshot spawnSnapshot{};
    spawnSnapshot.sequence = 1U;
    spawnSnapshot.serverTick = 10U;
    spawnSnapshot.count = 1U;
    spawnSnapshot.states[0] = {300U, 8U, 1U, {30.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}};
    std::vector<uint8_t> spawnBytes;
    if (!ReplicationSnapshotCodec::Serialize(spawnSnapshot, spawnBytes, codecError) || !ReplicationSnapshotCodec::Deserialize(spawnBytes, decoded, codecError) || !dynamicClient.ApplyServerSnapshot(decoded, apply) || apply.spawnedEntities != 1U || apply.appliedEntities != 1U || dynamicClient.RegisteredCount() != 1U || !dynamicClient.IsRegistered(300U)) return 29;
    ReplicationSnapshot despawnSnapshot{};
    despawnSnapshot.sequence = 2U;
    despawnSnapshot.serverTick = 11U;
    std::vector<uint8_t> despawnBytes;
    if (!ReplicationSnapshotCodec::Serialize(despawnSnapshot, despawnBytes, codecError) || !ReplicationSnapshotCodec::Deserialize(despawnBytes, decoded, codecError) || !dynamicClient.ApplyServerSnapshot(decoded, apply) || apply.despawnedEntities != 1U || dynamicClient.RegisteredCount() != 0U || dynamicClient.IsRegistered(300U)) return 30;
    if (!RunStaleRemoteEntityRegression()) return 31;
    return 0;
}
