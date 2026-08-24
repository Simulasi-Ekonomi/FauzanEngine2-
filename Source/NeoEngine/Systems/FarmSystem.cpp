#include "FarmSystem.h"
#include "TrustSafetySystem.h"

#include <algorithm>
#include <limits>

namespace NeoEngine {
namespace {
constexpr uint32_t kMagic = 0x4D524146U; // FARM
constexpr uint16_t kVersion = 1;

template <typename T>
void Append(std::vector<uint8_t>& output, T value) {
    for (size_t byte = 0; byte < sizeof(T); ++byte)
        output.push_back(static_cast<uint8_t>((static_cast<uint64_t>(value) >> (byte * 8U)) & 0xFFU));
}

template <typename T>
bool Read(std::span<const uint8_t> input, size_t& offset, T& value) {
    if (offset + sizeof(T) > input.size()) return false;
    uint64_t raw = 0;
    for (size_t byte = 0; byte < sizeof(T); ++byte)
        raw |= static_cast<uint64_t>(input[offset + byte]) << (byte * 8U);
    value = static_cast<T>(raw);
    offset += sizeof(T);
    return true;
}
} // namespace

FarmSystem::FarmSystem(uint16_t width, uint16_t height, int64_t initialCoins)
    : m_Width(width), m_Height(height), m_Coins(initialCoins) {
    const size_t tileCount = static_cast<size_t>(width) * height;
    if (width == 0 || height == 0 || tileCount > kMaxTiles || initialCoins < 0) {
        SetError(FarmError::InvalidConfiguration);
        return;
    }
    m_Tiles.resize(tileCount);
    m_Inventory[static_cast<size_t>(FarmItem::WheatSeed)] = 32;
    m_Inventory[static_cast<size_t>(FarmItem::CornSeed)] = 16;
    m_Inventory[static_cast<size_t>(FarmItem::TomatoSeed)] = 16;
    m_Ready = true;
    Touch();
}

bool FarmSystem::SetError(FarmError error) {
    if (error == FarmError::InsufficientInventory || error == FarmError::DuplicateTransaction || error == FarmError::AuthorityRejected)
        ++m_RejectedTransactionCount;
    m_LastError = error;
    return false;
}
void FarmSystem::SetTrustSafety(TrustSafetySystem* trustSafety, std::string playerId) { m_TrustSafety = trustSafety; m_TrustPlayerId = std::move(playerId); }

bool FarmSystem::IsCoordinateValid(uint16_t x, uint16_t z) const { return m_Ready && x < m_Width && z < m_Height; }
FarmSystem::Tile& FarmSystem::TileAt(uint16_t x, uint16_t z) { return m_Tiles[static_cast<size_t>(z) * m_Width + x]; }
const FarmSystem::Tile& FarmSystem::TileAt(uint16_t x, uint16_t z) const { return m_Tiles[static_cast<size_t>(z) * m_Width + x]; }
FarmTileState FarmSystem::TileStateAt(uint16_t x, uint16_t z) const { return IsCoordinateValid(x, z) ? TileAt(x, z).state : FarmTileState::Empty; }

FarmItem FarmSystem::SeedFor(FarmCrop crop) {
    switch (crop) { case FarmCrop::Wheat: return FarmItem::WheatSeed; case FarmCrop::Corn: return FarmItem::CornSeed; case FarmCrop::Tomato: return FarmItem::TomatoSeed; }
    return FarmItem::WheatSeed;
}
FarmItem FarmSystem::ProduceFor(FarmCrop crop) {
    switch (crop) { case FarmCrop::Wheat: return FarmItem::WheatProduce; case FarmCrop::Corn: return FarmItem::CornProduce; case FarmCrop::Tomato: return FarmItem::TomatoProduce; }
    return FarmItem::WheatProduce;
}
uint32_t FarmSystem::GrowthRequirement(FarmCrop crop) {
    switch (crop) { case FarmCrop::Wheat: return 24; case FarmCrop::Corn: return 36; case FarmCrop::Tomato: return 48; }
    return 48;
}

void FarmSystem::Touch() { ++m_StateRevision; m_LastError = FarmError::None; }
void FarmSystem::Emit(FarmEventType type, int64_t value) {
    if (m_RecentEvents.size() == kMaxEvents) m_RecentEvents.erase(m_RecentEvents.begin());
    m_RecentEvents.push_back({type, ++m_EventSequence, m_SimulationTick, value});
}

bool FarmSystem::AddItem(FarmItem item, uint32_t units) {
    const size_t slot = static_cast<size_t>(item);
    if (slot >= m_Inventory.size() || units > std::numeric_limits<uint32_t>::max() - m_Inventory[slot]) return SetError(FarmError::InventoryFull);
    m_Inventory[slot] += units;
    return true;
}
bool FarmSystem::RemoveItem(FarmItem item, uint32_t units) {
    const size_t slot = static_cast<size_t>(item);
    if (slot >= m_Inventory.size() || units == 0 || m_Inventory[slot] < units) return SetError(FarmError::InsufficientInventory);
    m_Inventory[slot] -= units;
    return true;
}

bool FarmSystem::Till(uint16_t x, uint16_t z) {
    if (!IsCoordinateValid(x, z)) return SetError(FarmError::InvalidCoordinate);
    Tile& tile = TileAt(x, z);
    if (tile.state != FarmTileState::Empty) return SetError(FarmError::InvalidAction);
    tile.state = FarmTileState::Tilled;
    Touch(); Emit(FarmEventType::Tilled); return true;
}
bool FarmSystem::Plant(uint16_t x, uint16_t z, FarmCrop crop) {
    if (!IsCoordinateValid(x, z)) return SetError(FarmError::InvalidCoordinate);
    Tile& tile = TileAt(x, z);
    if (tile.state != FarmTileState::Tilled) return SetError(FarmError::InvalidAction);
    if (!RemoveItem(SeedFor(crop), 1)) return false;
    tile.crop = crop; tile.growthTicks = 0; tile.watered = false; tile.state = FarmTileState::Growing;
    Touch(); Emit(FarmEventType::Planted); return true;
}
bool FarmSystem::Water(uint16_t x, uint16_t z) {
    if (!IsCoordinateValid(x, z)) return SetError(FarmError::InvalidCoordinate);
    Tile& tile = TileAt(x, z);
    if (tile.state != FarmTileState::Growing || tile.watered) return SetError(FarmError::InvalidAction);
    tile.watered = true;
    Touch(); Emit(FarmEventType::Watered); return true;
}
bool FarmSystem::Tick(uint32_t ticks) {
    if (!m_Ready || ticks == 0) return SetError(FarmError::InvalidAction);
    if (ticks > std::numeric_limits<uint64_t>::max() - m_SimulationTick) return SetError(FarmError::InvalidAction);
    m_SimulationTick += ticks;
    for (Tile& tile : m_Tiles) {
        if (tile.state != FarmTileState::Growing) continue;
        const uint64_t growth = static_cast<uint64_t>(tile.growthTicks) + static_cast<uint64_t>(ticks) * (tile.watered ? 2U : 1U);
        tile.growthTicks = static_cast<uint32_t>(std::min<uint64_t>(growth, GrowthRequirement(tile.crop)));
        if (tile.growthTicks >= GrowthRequirement(tile.crop)) tile.state = FarmTileState::Harvestable;
    }
    for (AnimalState& animal : m_Animals) {
        animal.productionTicks += ticks;
        if (animal.animal == FarmAnimal::Hen && animal.productionTicks >= 12) {
            const uint32_t output = animal.productionTicks / 12;
            animal.productionTicks %= 12;
            if (!AddItem(FarmItem::Egg, output)) return false;
            Emit(FarmEventType::AnimalProduced, output);
        }
    }
    Touch(); return true;
}
bool FarmSystem::Harvest(uint16_t x, uint16_t z, uint32_t& harvestedUnits) {
    harvestedUnits = 0;
    if (!IsCoordinateValid(x, z)) return SetError(FarmError::InvalidCoordinate);
    Tile& tile = TileAt(x, z);
    if (tile.state != FarmTileState::Harvestable) return SetError(FarmError::InvalidAction);
    if (!AddItem(ProduceFor(tile.crop), 2)) return false;
    harvestedUnits = 2;
    tile = {};
    ++m_QuestHarvestProgress;
    if (!m_QuestCompleted && m_QuestHarvestProgress >= kQuestHarvestTarget) { m_QuestCompleted = true; Emit(FarmEventType::QuestCompleted); }
    Touch(); Emit(FarmEventType::Harvested, harvestedUnits); return true;
}
bool FarmSystem::AddAnimal(FarmAnimal animal) {
    if (!m_Ready || m_Animals.size() >= 128) return SetError(FarmError::InvalidAction);
    m_Animals.push_back({animal, 0}); Touch(); return true;
}
bool FarmSystem::Sell(uint64_t saleId, FarmItem item, uint32_t units, int64_t pricePerUnit) {
    if (m_TrustSafety && m_TrustSafety->IsBanned(m_TrustPlayerId)) return SetError(FarmError::Banned);
    if (!m_Ready || saleId == 0 || units == 0 || pricePerUnit <= 0) return SetError(FarmError::InvalidAction);
    if (m_AppliedSaleIds.contains(saleId)) { if (m_TrustSafety) m_TrustSafety->Report(m_TrustPlayerId, "sale-" + std::to_string(saleId), FraudSignal::DuplicateReceipt); return SetError(FarmError::DuplicateTransaction); }
    if (m_AppliedSaleIds.size() >= kMaxLedgerEntries) return SetError(FarmError::LedgerCapacityReached);
    const int64_t credit = static_cast<int64_t>(units) * pricePerUnit;
    if (credit <= 0 || credit > std::numeric_limits<int64_t>::max() - m_Coins) return SetError(FarmError::InvalidAction);
    if (ItemCount(item) < units) { if (m_TrustSafety) m_TrustSafety->Report(m_TrustPlayerId, "inventory-" + std::to_string(saleId), FraudSignal::ImpossibleInventory); return SetError(FarmError::InsufficientInventory); }
    if (!RemoveItem(item, units)) return false;
    m_AppliedSaleIds.insert(saleId); m_Coins += credit; Touch(); Emit(FarmEventType::Sold, credit); return true;
}
bool FarmSystem::ApplyVerifiedTopUp(const VerifiedTopUpReceipt& receipt) {
    if (m_TrustSafety && m_TrustSafety->IsBanned(m_TrustPlayerId)) return SetError(FarmError::Banned);
    if (!m_Ready || receipt.receiptId == 0 || receipt.amount <= 0 || !m_ReceiptVerifier || !m_ReceiptVerifier(receipt)) return SetError(FarmError::AuthorityRejected);
    if (m_AppliedReceiptIds.contains(receipt.receiptId)) { if (m_TrustSafety) m_TrustSafety->Report(m_TrustPlayerId, "receipt-" + std::to_string(receipt.receiptId), FraudSignal::DuplicateReceipt); return SetError(FarmError::DuplicateTransaction); }
    if (m_AppliedReceiptIds.size() >= kMaxLedgerEntries || receipt.amount > std::numeric_limits<int64_t>::max() - m_Coins) return SetError(FarmError::LedgerCapacityReached);
    m_AppliedReceiptIds.insert(receipt.receiptId); m_Coins += receipt.amount; Touch(); Emit(FarmEventType::TopUpAccepted, receipt.amount); return true;
}
uint32_t FarmSystem::ItemCount(FarmItem item) const { const size_t slot = static_cast<size_t>(item); return slot < m_Inventory.size() ? m_Inventory[slot] : 0; }
FarmTelemetrySnapshot FarmSystem::Snapshot() const {
    FarmTelemetrySnapshot snapshot{};
    snapshot.simulationTick = m_SimulationTick; snapshot.stateRevision = m_StateRevision; snapshot.eventSequence = m_EventSequence; snapshot.coins = m_Coins;
    snapshot.animals = static_cast<uint32_t>(m_Animals.size()); snapshot.questHarvestProgress = m_QuestHarvestProgress; snapshot.questCompleted = m_QuestCompleted; snapshot.lastError = m_LastError;
    for (const Tile& tile : m_Tiles) { if (tile.state == FarmTileState::Tilled) ++snapshot.tilledTiles; if (tile.state == FarmTileState::Growing) ++snapshot.growingTiles; if (tile.state == FarmTileState::Harvestable) ++snapshot.harvestableTiles; }
    return snapshot;
}

std::vector<uint8_t> FarmSystem::Serialize() const {
    std::vector<uint8_t> output; output.reserve(64 + m_Tiles.size() * 8);
    Append<uint32_t>(output, kMagic); Append<uint16_t>(output, kVersion); Append<uint16_t>(output, m_Width); Append<uint16_t>(output, m_Height);
    Append<int64_t>(output, m_Coins); Append<uint64_t>(output, m_SimulationTick); Append<uint64_t>(output, m_StateRevision); Append<uint64_t>(output, m_EventSequence);
    for (uint32_t value : m_Inventory) Append<uint32_t>(output, value);
    for (const Tile& tile : m_Tiles) { Append<uint8_t>(output, static_cast<uint8_t>(tile.state)); Append<uint8_t>(output, static_cast<uint8_t>(tile.crop)); Append<uint32_t>(output, tile.growthTicks); Append<uint8_t>(output, tile.watered ? 1U : 0U); }
    Append<uint32_t>(output, static_cast<uint32_t>(m_Animals.size())); for (const AnimalState& animal : m_Animals) { Append<uint8_t>(output, static_cast<uint8_t>(animal.animal)); Append<uint32_t>(output, animal.productionTicks); }
    Append<uint32_t>(output, m_QuestHarvestProgress); Append<uint8_t>(output, m_QuestCompleted ? 1U : 0U); Append<uint32_t>(output, m_RejectedTransactionCount);
    auto appendLedger = [&output](const std::unordered_set<uint64_t>& ledger) { std::vector<uint64_t> ids(ledger.begin(), ledger.end()); std::sort(ids.begin(), ids.end()); Append<uint32_t>(output, static_cast<uint32_t>(ids.size())); for (uint64_t id : ids) Append<uint64_t>(output, id); };
    appendLedger(m_AppliedReceiptIds); appendLedger(m_AppliedSaleIds); return output;
}

bool FarmSystem::Deserialize(std::span<const uint8_t> bytes) {
    size_t offset = 0; uint32_t magic = 0; uint16_t version = 0, width = 0, height = 0; int64_t coins = 0; uint64_t tick = 0, revision = 0, eventSequence = 0;
    if (!Read(bytes, offset, magic) || !Read(bytes, offset, version) || !Read(bytes, offset, width) || !Read(bytes, offset, height) || magic != kMagic || version != kVersion || static_cast<size_t>(width) * height > kMaxTiles || width == 0 || height == 0 || !Read(bytes, offset, coins) || !Read(bytes, offset, tick) || !Read(bytes, offset, revision) || !Read(bytes, offset, eventSequence) || coins < 0) return SetError(FarmError::CorruptPersistence);
    std::array<uint32_t, static_cast<size_t>(FarmItem::Count)> inventory{}; for (uint32_t& value : inventory) if (!Read(bytes, offset, value)) return SetError(FarmError::CorruptPersistence);
    std::vector<Tile> tiles(static_cast<size_t>(width) * height); for (Tile& tile : tiles) { uint8_t state = 0, crop = 0, watered = 0; if (!Read(bytes, offset, state) || !Read(bytes, offset, crop) || !Read(bytes, offset, tile.growthTicks) || !Read(bytes, offset, watered) || state > static_cast<uint8_t>(FarmTileState::Harvestable) || crop > static_cast<uint8_t>(FarmCrop::Tomato) || watered > 1) return SetError(FarmError::CorruptPersistence); tile.state = static_cast<FarmTileState>(state); tile.crop = static_cast<FarmCrop>(crop); tile.watered = watered != 0; }
    uint32_t animalCount = 0; if (!Read(bytes, offset, animalCount) || animalCount > 128) return SetError(FarmError::CorruptPersistence); std::vector<AnimalState> animals(animalCount); for (AnimalState& animal : animals) { uint8_t type = 0; if (!Read(bytes, offset, type) || !Read(bytes, offset, animal.productionTicks) || type > static_cast<uint8_t>(FarmAnimal::Hen)) return SetError(FarmError::CorruptPersistence); animal.animal = static_cast<FarmAnimal>(type); }
    uint32_t quest = 0, rejectedTransactions = 0; uint8_t completed = 0; if (!Read(bytes, offset, quest) || !Read(bytes, offset, completed) || !Read(bytes, offset, rejectedTransactions) || completed > 1) return SetError(FarmError::CorruptPersistence);
    auto readLedger = [&bytes, &offset](std::unordered_set<uint64_t>& ledger) { uint32_t count = 0; if (!Read(bytes, offset, count) || count > kMaxLedgerEntries) return false; for (uint32_t i = 0; i < count; ++i) { uint64_t id = 0; if (!Read(bytes, offset, id) || id == 0 || !ledger.insert(id).second) return false; } return true; };
    std::unordered_set<uint64_t> receipts, sales; if (!readLedger(receipts) || !readLedger(sales) || offset != bytes.size()) return SetError(FarmError::CorruptPersistence);
    m_Width = width; m_Height = height; m_Coins = coins; m_SimulationTick = tick; m_StateRevision = revision; m_EventSequence = eventSequence; m_Inventory = inventory; m_Tiles = std::move(tiles); m_Animals = std::move(animals); m_QuestHarvestProgress = quest; m_QuestCompleted = completed != 0; m_RejectedTransactionCount = rejectedTransactions; m_AppliedReceiptIds = std::move(receipts); m_AppliedSaleIds = std::move(sales); m_RecentEvents.clear(); m_Ready = true; m_LastError = FarmError::None; return true;
}

} // namespace NeoEngine
