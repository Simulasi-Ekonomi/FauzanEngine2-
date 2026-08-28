#include "FarmTelemetryAdapter.h"

#include <algorithm>
#include <sstream>

namespace NeoEngine {
namespace {
std::string EscapeJson(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    for (unsigned char c : input) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (c >= 0x20U) result += static_cast<char>(c);
                else return {};
                break;
        }
    }
    return result;
}

const char* EventTypeName(FarmEventType type) {
    switch (type) {
        case FarmEventType::Tilled: return "farm.tilled";
        case FarmEventType::Planted: return "farm.planted";
        case FarmEventType::Watered: return "farm.watered";
        case FarmEventType::Harvested: return "farm.harvested";
        case FarmEventType::Sold: return "farm.sold";
        case FarmEventType::TopUpAccepted: return "farm.topup_accepted";
        case FarmEventType::AnimalProduced: return "farm.animal_produced";
        case FarmEventType::QuestCompleted: return "farm.quest_completed";
    }
    return "farm.unknown";
}
} // namespace

bool FarmTelemetryAdapter::IsIdentityValid(const FarmTelemetryIdentity& identity) {
    return identity.sourceRef.size() >= 3 && identity.sourceRef.size() <= 96 && identity.gameKey.size() >= 1 &&
           identity.gameKey.size() <= 96 && identity.engineVersion.size() >= 1 && identity.engineVersion.size() <= 96 &&
           identity.playerId.size() >= 1 && identity.playerId.size() <= 96;
}

bool FarmTelemetryAdapter::BuildEnvelope(const FarmSystem& farm, uint64_t occurredAtMs, std::string& json) const {
    json.clear();
    if (!farm.IsReady() || occurredAtMs == 0 || !IsIdentityValid(m_Identity) || m_Policy.maxEvents > 64U) return false;
    const FarmTelemetrySnapshot snapshot = farm.Snapshot();
    const std::string source = EscapeJson(m_Identity.sourceRef);
    const std::string game = EscapeJson(m_Identity.gameKey);
    const std::string version = EscapeJson(m_Identity.engineVersion);
    const std::string player = EscapeJson(m_Identity.playerId);
    if (source.empty() || game.empty() || version.empty() || player.empty()) return false;
    const uint64_t tileCount = static_cast<uint64_t>(farm.Width()) * farm.Height();
    const uint64_t planted = static_cast<uint64_t>(snapshot.growingTiles) + snapshot.harvestableTiles;
    const std::string snapshotRef = m_Identity.sourceRef + "-" + std::to_string(snapshot.stateRevision) + "-" +
                                     std::to_string(snapshot.simulationTick);
    if (snapshotRef.size() < 8 || snapshotRef.size() > 128) return false;

    std::ostringstream output;
    output << "{\"schemaVersion\":1,\"sourceRef\":\"" << source << "\",\"gameKey\":\"" << game
           << "\",\"engineVersion\":\"" << version << "\",\"snapshotRef\":\"" << EscapeJson(snapshotRef)
           << "\",\"occurredAtMs\":" << occurredAtMs;
    if (m_Policy.includePlayerId) output << ",\"playerId\":\"" << player << '\"';
    output << ",\"telemetry\":{\"tileCount\":" << tileCount << ",\"plantedTiles\":" << planted
           << ",\"harvestableTiles\":" << snapshot.harvestableTiles << ",\"animalCount\":" << snapshot.animals
           << ",\"animalProductsReady\":" << farm.ItemCount(FarmItem::Egg)
           << ",\"verifiedReceipts\":" << farm.AcceptedReceiptCount()
           << ",\"rejectedTransactions\":" << farm.RejectedTransactionCount();
    if (m_Policy.includeEconomicValues) output << ",\"goldBalance\":" << snapshot.coins;
    output << "},\"events\":[";

    const auto& events = farm.RecentEvents();
    const size_t eventCount = std::min<size_t>(events.size(), m_Policy.maxEvents);
    for (size_t index = 0; index < eventCount; ++index) {
        const FarmEvent& event = events[index];
        if (index != 0U) output << ',';
        output << "{\"eventRef\":\"" << EscapeJson(snapshotRef + "-e-" + std::to_string(event.sequence))
               << "\",\"eventType\":\"" << EventTypeName(event.type) << "\",\"occurredAtMs\":" << occurredAtMs;
        if (m_Policy.includeEventValues) {
            output << ",\"detail\":\"value=" << event.value << ";tick=" << event.simulationTick << '"';
        }
        output << '}';
    }
    output << "]}";
    json = output.str();
    return json.size() <= 65536U;
}

bool FarmTelemetryAdapter::BuildWorldEnvelope(const FarmSystem& farm, const FarmWorldTool& world, uint64_t occurredAtMs,
                                              std::string& json) const {
    if (!world.IsReady() || !BuildEnvelope(farm, occurredAtMs, json)) return false;
    const FarmWorldSnapshot worldSnapshot = world.Snapshot();
    const std::string marker = ",\"events\":[";
    const size_t insertion = json.find(marker);
    if (insertion == std::string::npos) {
        json.clear();
        return false;
    }
    std::ostringstream worldJson;
    worldJson << ",\"world\":{\"width\":" << worldSnapshot.worldWidth << ",\"height\":" << worldSnapshot.worldHeight
              << ",\"buildings\":" << worldSnapshot.buildings << ",\"npcs\":" << worldSnapshot.npcs
              << ",\"quests\":" << worldSnapshot.quests << ",\"unusedPermits\":" << worldSnapshot.unusedPermits
              << ",\"governmentLedgerEvents\":" << worldSnapshot.governmentLedgerEvents
              << ",\"simulationTick\":" << worldSnapshot.simulationTick;
    if (m_Policy.includeEconomicValues) worldJson << ",\"observedFarmCoins\":" << worldSnapshot.observedFarmCoins;
    worldJson << '}';
    json.insert(insertion, worldJson.str());
    if (json.size() > 65536U) {
        json.clear();
        return false;
    }
    return true;
}

} // namespace NeoEngine
