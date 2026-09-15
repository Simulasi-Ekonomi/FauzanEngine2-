#include "../Source/NeoEngine/Runtime/AtomicSaveFile.h"
#include "../Source/NeoEngine/Systems/TelemetryOutbox.h"
#include "../Source/NeoEngine/Systems/CommodityEconomyLedger.h"
#include "../Source/NeoEngine/Systems/CommodityCatalog.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int main() {
    std::cout << "====================================================\n";
    std::cout << "[ENGINE SMOKE TEST] Canonical runtime verification\n";
    std::cout << "====================================================\n";

    // 1. Atomic persistence: write/read plus backup/restore.
    std::cout << "[1/3] Testing atomic persistence and recovery...\n";
    const std::filesystem::path root = "/tmp/fauzan_engine_comprehensive_smoke";
    const std::string slot = "canonical_slot";
    const std::vector<uint8_t> payload = {0x46, 0x5A, 0x45, 0x31, 0x01, 0x02, 0x03, 0x04};
    NeoEngine::AtomicSaveFileError error = NeoEngine::AtomicSaveFileError::None;

    if (!NeoEngine::AtomicSaveFile::Write(root, slot, payload, error)) {
        std::cerr << "FAIL: AtomicSaveFile write failed.\n";
        return 1;
    }

    std::vector<uint8_t> loaded;
    if (!NeoEngine::AtomicSaveFile::Read(root, slot, loaded, error) || loaded != payload) {
        std::cerr << "FAIL: AtomicSaveFile round-trip mismatch.\n";
        return 1;
    }

    const std::vector<uint8_t> replacement = {0xAA, 0xBB, 0xCC};
    if (!NeoEngine::AtomicSaveFile::Backup(root, slot, error) ||
        !NeoEngine::AtomicSaveFile::Write(root, slot, replacement, error) ||
        !NeoEngine::AtomicSaveFile::RestoreBackup(root, slot, error)) {
        std::cerr << "FAIL: AtomicSaveFile backup/restore failed.\n";
        return 1;
    }

    loaded.clear();
    if (!NeoEngine::AtomicSaveFile::Read(root, slot, loaded, error) || loaded != payload) {
        std::cerr << "FAIL: AtomicSaveFile restore did not recover original payload.\n";
        return 1;
    }

    // 2. Telemetry durability: queue, acknowledgement and serialization.
    std::cout << "[2/3] Testing telemetry outbox persistence...\n";
    NeoEngine::TelemetryOutbox outbox;
    if (!outbox.Enqueue("evt_001", "{\"type\":\"login\"}") ||
        !outbox.Enqueue("evt_002", "{\"type\":\"purchase\"}") ||
        outbox.Pending().size() != 2) {
        std::cerr << "FAIL: TelemetryOutbox enqueue failed.\n";
        return 1;
    }

    const std::vector<uint8_t> telemetryBytes = outbox.Serialize();
    NeoEngine::TelemetryOutbox restoredOutbox;
    if (telemetryBytes.empty() || !restoredOutbox.Deserialize(telemetryBytes) ||
        restoredOutbox.Pending().size() != 2) {
        std::cerr << "FAIL: TelemetryOutbox serialization round-trip failed.\n";
        return 1;
    }

    if (!restoredOutbox.Acknowledge("evt_001") || restoredOutbox.Pending().size() != 1) {
        std::cerr << "FAIL: TelemetryOutbox acknowledgement failed.\n";
        return 1;
    }

    // 3. Economy command integrity: apply, accounting and idempotency.
    std::cout << "[3/3] Testing economy command integrity...\n";
    NeoEngine::CommodityCatalog catalog;
    NeoEngine::CommodityDefinition commodity;
    commodity.id = "C001";
    commodity.category = NeoEngine::CommodityCategory::Crop;
    commodity.baseUnitPrice = 50;
    commodity.maxStack = 999;
    commodity.tradable = true;
    catalog.Add(commodity);

    NeoEngine::CommodityEconomyLedger ledger;
    NeoEngine::CommodityEconomyCommand command;
    command.id = "cmd_001";
    command.kind = NeoEngine::CommodityEconomyKind::HarvestGrant;
    command.commodityId = "C001";
    command.quantity = 10;

    if (!ledger.Apply(catalog, command) || ledger.Quantity("C001") != 10 ||
        !ledger.HasApplied("cmd_001")) {
        std::cerr << "FAIL: Economy command application failed.\n";
        return 1;
    }

    if (ledger.Apply(catalog, command) || ledger.Quantity("C001") != 10) {
        std::cerr << "FAIL: Economy command idempotency failed.\n";
        return 1;
    }

    std::cout << "====================================================\n";
    std::cout << "ENGINE CANONICAL SMOKE TESTS PASSED\n";
    std::cout << "====================================================\n";
    return 0;
}
