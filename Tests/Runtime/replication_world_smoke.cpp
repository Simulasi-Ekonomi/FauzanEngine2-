#include "Runtime/ReplicationWorld.h"

#include <cmath>
#include <vector>

int main() {
    using namespace NeoEngine;
    SceneWorld serverScene;
    SceneWorld clientScene;
    SceneEntity serverLocal{}, serverRemote{}, clientLocal{}, clientRemote{};
    if (!serverScene.Create(serverLocal) || !serverScene.Create(serverRemote) || !clientScene.Create(clientLocal) || !clientScene.Create(clientRemote)) return 1;
    if (!serverScene.SetTransform(serverLocal, {5.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}) || !serverScene.SetTransform(serverRemote, {20.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}) || !clientScene.SetTransform(clientLocal, {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F}) || !clientScene.SetTransform(clientRemote, {10.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F})) return 2;

    ReplicationWorld server(serverScene, ReplicationRole::Server, 0U);
    ReplicationWorld client(clientScene, ReplicationRole::Client, 7U);
    if (!server.RegisterEntity(serverLocal, 100U, 7U) || !server.RegisterEntity(serverRemote, 200U, 8U) || !client.RegisterEntity(clientLocal, 100U, 7U) || !client.RegisterEntity(clientRemote, 200U, 8U)) return 3;
    if (client.RegisterEntity(clientLocal, 200U, 7U) || client.LastError() != ReplicationError::DuplicateNetworkId) return 4;
    if (server.RegisterEntity(serverLocal, 0U, 7U) || server.LastError() != ReplicationError::InvalidNetworkId || !serverScene.SetTransform(serverLocal, {6.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F})) return 5;

    ReplicationSnapshot snapshot{};
    if (!server.BuildServerSnapshot(1U, snapshot) || snapshot.count != 2U || snapshot.sequence != 1U || snapshot.states[0].networkId != 100U || snapshot.states[1].networkId != 200U) return 6;
    std::vector<uint8_t> encoded;
    ReplicationError codecError = ReplicationError::None;
    if (!ReplicationSnapshotCodec::Serialize(snapshot, encoded, codecError) || encoded.empty() || codecError != ReplicationError::None) return 7;
    ReplicationSnapshot decoded{};
    if (!ReplicationSnapshotCodec::Deserialize(encoded, decoded, codecError) || decoded.sequence != snapshot.sequence || decoded.states[1].transform.x != 20.0F) return 8;
    encoded.back() ^= 0x4FU;
    if (ReplicationSnapshotCodec::Deserialize(encoded, decoded, codecError) || codecError != ReplicationError::CorruptSnapshot) return 9;

    ReplicationPredictionReceipt prediction{};
    if (!client.PredictLocalInput(100U, 1.0F, 0.0F, prediction)) return 10;
    ReplicationApplyReceipt apply{};
    if (!client.ApplyServerSnapshot(snapshot, apply) || !apply.accepted || apply.appliedEntities != 2U || apply.reconciledPredictions != 1U || client.SnapshotSequence() != 1U) return 11;
    const Transform3* localAfterReconcile = clientScene.GetTransform(clientLocal);
    const Transform3* remoteBeforeInterpolation = clientScene.GetTransform(clientRemote);
    if (localAfterReconcile == nullptr || std::abs(localAfterReconcile->x - 6.0F) > 0.0001F || remoteBeforeInterpolation == nullptr || std::abs(remoteBeforeInterpolation->x - 10.0F) > 0.0001F) return 12;
    ReplicationSnapshot unknownEntitySnapshot = snapshot;
    unknownEntitySnapshot.sequence = 3U;
    unknownEntitySnapshot.states[0].transform.x = 123.0F;
    unknownEntitySnapshot.states[1].networkId = 999U;
    std::vector<uint8_t> unknownEntityBytes;
    if (!ReplicationSnapshotCodec::Serialize(unknownEntitySnapshot, unknownEntityBytes, codecError) || !ReplicationSnapshotCodec::Deserialize(unknownEntityBytes, decoded, codecError) || client.ApplyServerSnapshot(decoded, apply) || client.LastError() != ReplicationError::UnknownEntity) return 13;
    localAfterReconcile = clientScene.GetTransform(clientLocal);
    if (localAfterReconcile == nullptr || std::abs(localAfterReconcile->x - 6.0F) > 0.0001F) return 14;
    if (!client.SetInterpolationAlphaPermille(500U) || !client.ApplyInterpolation(apply) || apply.interpolatedEntities != 1U) return 15;
    const Transform3* remoteAfterInterpolation = clientScene.GetTransform(clientRemote);
    if (remoteAfterInterpolation == nullptr || std::abs(remoteAfterInterpolation->x - 15.0F) > 0.0001F) return 14;
    if (client.ApplyServerSnapshot(snapshot, apply) || client.LastError() != ReplicationError::StaleSnapshot) return 15;
    ReplicationSnapshot perEntityStale = snapshot; perEntityStale.sequence = 2U; perEntityStale.states[0].stateRevision = 0U;
    std::vector<uint8_t> staleBytes;
    if (!ReplicationSnapshotCodec::Serialize(perEntityStale, staleBytes, codecError) || !ReplicationSnapshotCodec::Deserialize(staleBytes, decoded, codecError) || client.ApplyServerSnapshot(decoded, apply) || client.LastError() != ReplicationError::StaleSnapshot) return 16;
    if (client.PredictLocalInput(200U, 1.0F, 0.0F, prediction) || client.LastError() != ReplicationError::OwnershipRejected) return 17;
    if (client.SetInterpolationAlphaPermille(1001U) || client.LastError() != ReplicationError::InvalidInput) return 18;
    if (server.ApplyServerSnapshot(snapshot, apply) || server.LastError() != ReplicationError::NotClient || server.BuildServerSnapshot(2U, snapshot) == false) return 19;
    if (!client.UnregisterEntity(200U) || client.IsRegistered(200U) || client.RegisteredCount() != 1U) return 20;
    return 0;
}
