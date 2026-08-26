#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

namespace NeoEngine {
class TrustSafetySystem;

enum class FarmError : uint8_t {
    None,
    InvalidConfiguration,
    InvalidCoordinate,
    InvalidAction,
    InventoryFull,
    InsufficientInventory,
    DuplicateTransaction,
    AuthorityRejected,
    Banned,
    LedgerCapacityReached,
    CorruptPersistence,
};

enum class FarmCrop : uint8_t { Wheat, Corn, Tomato };
enum class FarmAnimal : uint8_t { Hen };
enum class FarmItem : uint8_t { WheatSeed, WheatProduce, CornSeed, CornProduce, TomatoSeed, TomatoProduce, Egg, Count };
enum class FarmTileState : uint8_t { Empty, Tilled, Growing, Harvestable };
enum class FarmEventType : uint8_t { Tilled, Planted, Watered, Harvested, Sold, TopUpAccepted, AnimalProduced, QuestCompleted };

struct VerifiedTopUpReceipt {
    uint64_t receiptId = 0;
    int64_t amount = 0;
    std::string authorityPayload;
};

struct FarmEvent {
    FarmEventType type = FarmEventType::Tilled;
    uint64_t sequence = 0;
    uint64_t simulationTick = 0;
    int64_t value = 0;
};

struct FarmTelemetrySnapshot {
    uint64_t simulationTick = 0;
    uint64_t stateRevision = 0;
    uint64_t eventSequence = 0;
    int64_t coins = 0;
    uint32_t tilledTiles = 0;
    uint32_t growingTiles = 0;
    uint32_t harvestableTiles = 0;
    uint32_t animals = 0;
    uint32_t questHarvestProgress = 0;
    bool questCompleted = false;
    FarmError lastError = FarmError::None;
};

class FarmSystem {
public:
    static constexpr size_t kMaxTiles = 1000;
    static constexpr size_t kMaxLedgerEntries = 4096;
    static constexpr size_t kMaxEvents = 256;

    using ReceiptVerifier = std::function<bool(const VerifiedTopUpReceipt&)>;

    FarmSystem(uint16_t width, uint16_t height, int64_t initialCoins = 100);

    bool IsReady() const { return m_Ready; }
    FarmError LastError() const { return m_LastError; }
    uint16_t Width() const { return m_Width; }
    uint16_t Height() const { return m_Height; }
    int64_t Coins() const { return m_Coins; }
    uint64_t SimulationTick() const { return m_SimulationTick; }
    FarmTileState TileStateAt(uint16_t x, uint16_t z) const;
    bool IsWateredAt(uint16_t x, uint16_t z) const;
    uint32_t AcceptedReceiptCount() const { return static_cast<uint32_t>(m_AppliedReceiptIds.size()); }
    uint32_t RejectedTransactionCount() const { return m_RejectedTransactionCount; }

    bool Till(uint16_t x, uint16_t z);
    bool Plant(uint16_t x, uint16_t z, FarmCrop crop);
    bool Water(uint16_t x, uint16_t z);
    bool Tick(uint32_t ticks);
    bool Harvest(uint16_t x, uint16_t z, uint32_t& harvestedUnits);

    bool AddAnimal(FarmAnimal animal);
    bool Sell(uint64_t saleId, FarmItem item, uint32_t units, int64_t pricePerUnit);
    bool ApplyVerifiedTopUp(const VerifiedTopUpReceipt& receipt);
    void SetReceiptVerifier(ReceiptVerifier verifier) { m_ReceiptVerifier = std::move(verifier); }
    void SetTrustSafety(TrustSafetySystem* trustSafety, std::string playerId);

    uint32_t ItemCount(FarmItem item) const;
    FarmTelemetrySnapshot Snapshot() const;
    std::vector<FarmEvent> RecentEvents() const { return m_RecentEvents; }

    std::vector<uint8_t> Serialize() const;
    bool Deserialize(std::span<const uint8_t> bytes);

private:
    struct Tile {
        FarmTileState state = FarmTileState::Empty;
        FarmCrop crop = FarmCrop::Wheat;
        uint32_t growthTicks = 0;
        bool watered = false;
    };

    struct AnimalState {
        FarmAnimal animal = FarmAnimal::Hen;
        uint32_t productionTicks = 0;
    };

    bool SetError(FarmError error);
    bool IsCoordinateValid(uint16_t x, uint16_t z) const;
    Tile& TileAt(uint16_t x, uint16_t z);
    const Tile& TileAt(uint16_t x, uint16_t z) const;
    static FarmItem SeedFor(FarmCrop crop);
    static FarmItem ProduceFor(FarmCrop crop);
    static uint32_t GrowthRequirement(FarmCrop crop);
    bool AddItem(FarmItem item, uint32_t units);
    bool RemoveItem(FarmItem item, uint32_t units);
    void Emit(FarmEventType type, int64_t value = 0);
    void Touch();

    uint16_t m_Width = 0;
    uint16_t m_Height = 0;
    bool m_Ready = false;
    FarmError m_LastError = FarmError::None;
    int64_t m_Coins = 0;
    uint64_t m_SimulationTick = 0;
    uint64_t m_StateRevision = 0;
    uint64_t m_EventSequence = 0;
    std::array<uint32_t, static_cast<size_t>(FarmItem::Count)> m_Inventory{};
    std::vector<Tile> m_Tiles;
    std::vector<AnimalState> m_Animals;
    std::unordered_set<uint64_t> m_AppliedReceiptIds;
    std::unordered_set<uint64_t> m_AppliedSaleIds;
    uint32_t m_QuestHarvestProgress = 0;
    uint32_t m_RejectedTransactionCount = 0;
    static constexpr uint32_t kQuestHarvestTarget = 5;
    bool m_QuestCompleted = false;
    std::vector<FarmEvent> m_RecentEvents;
    ReceiptVerifier m_ReceiptVerifier;
    TrustSafetySystem* m_TrustSafety = nullptr;
    std::string m_TrustPlayerId;
};

} // namespace NeoEngine
