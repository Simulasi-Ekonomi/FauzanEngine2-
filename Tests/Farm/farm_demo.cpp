#include "Systems/FarmSystem.h"

#include <iostream>
#include <sstream>
#include <string>

using namespace NeoEngine;

namespace {
const char* ErrorName(FarmError error) {
    switch (error) {
        case FarmError::None: return "none";
        case FarmError::InvalidConfiguration: return "invalid_configuration";
        case FarmError::InvalidCoordinate: return "invalid_coordinate";
        case FarmError::InvalidAction: return "invalid_action";
        case FarmError::InventoryFull: return "inventory_full";
        case FarmError::InsufficientInventory: return "insufficient_inventory";
        case FarmError::DuplicateTransaction: return "duplicate_transaction";
        case FarmError::AuthorityRejected: return "authority_rejected";
        case FarmError::LedgerCapacityReached: return "ledger_capacity_reached";
        case FarmError::CorruptPersistence: return "corrupt_persistence";
    }
    return "unknown";
}

bool ParseCrop(const std::string& input, FarmCrop& crop) {
    if (input == "wheat") { crop = FarmCrop::Wheat; return true; }
    if (input == "corn") { crop = FarmCrop::Corn; return true; }
    if (input == "tomato") { crop = FarmCrop::Tomato; return true; }
    return false;
}

bool ParseProduce(const std::string& input, FarmItem& item) {
    if (input == "wheat") { item = FarmItem::WheatProduce; return true; }
    if (input == "corn") { item = FarmItem::CornProduce; return true; }
    if (input == "tomato") { item = FarmItem::TomatoProduce; return true; }
    if (input == "egg") { item = FarmItem::Egg; return true; }
    return false;
}

char TileGlyph(FarmTileState state) {
    switch (state) {
        case FarmTileState::Empty: return '.';
        case FarmTileState::Tilled: return 'T';
        case FarmTileState::Growing: return 'G';
        case FarmTileState::Harvestable: return 'H';
    }
    return '?';
}

void PrintStatus(const FarmSystem& farm) {
    const FarmTelemetrySnapshot snapshot = farm.Snapshot();
    std::cout << "tick=" << snapshot.simulationTick << " coins=" << snapshot.coins
              << " wheat=" << farm.ItemCount(FarmItem::WheatProduce)
              << " corn=" << farm.ItemCount(FarmItem::CornProduce)
              << " tomato=" << farm.ItemCount(FarmItem::TomatoProduce)
              << " eggs=" << farm.ItemCount(FarmItem::Egg)
              << " quest=" << snapshot.questHarvestProgress << (snapshot.questCompleted ? "/done" : "/5")
              << " tiles(t/g/h)=" << snapshot.tilledTiles << '/' << snapshot.growingTiles << '/' << snapshot.harvestableTiles << '\n';
    for (uint16_t z = 0; z < farm.Height(); ++z) {
        for (uint16_t x = 0; x < farm.Width(); ++x) std::cout << TileGlyph(farm.TileStateAt(x, z));
        std::cout << '\n';
    }
}

void PrintHelp() {
    std::cout << "commands: status | till X Z | plant X Z wheat|corn|tomato | water X Z | tick N | harvest X Z | animal hen | sell ID wheat|corn|tomato|egg UNITS PRICE | help | quit\n";
}
} // namespace

int main(int argc, char** argv) {
    const bool scripted = argc > 1 && std::string(argv[1]) == "--scripted";
    FarmSystem farm(8, 8);
    std::cout << "FAUZANENGINE FARM DEMO\n";
    PrintHelp();
    PrintStatus(farm);
    std::string line;
    while (true) {
        if (!scripted) std::cout << "farm> " << std::flush;
        if (!std::getline(std::cin, line)) break;
        std::istringstream input(line);
        std::string command;
        input >> command;
        if (command.empty()) continue;
        if (command == "quit" || command == "exit") break;
        if (command == "help") { PrintHelp(); continue; }
        if (command == "status") { PrintStatus(farm); continue; }
        bool success = false;
        if (command == "till") { unsigned x = 0, z = 0; success = static_cast<bool>(input >> x >> z) && farm.Till(static_cast<uint16_t>(x), static_cast<uint16_t>(z)); }
        else if (command == "water") { unsigned x = 0, z = 0; success = static_cast<bool>(input >> x >> z) && farm.Water(static_cast<uint16_t>(x), static_cast<uint16_t>(z)); }
        else if (command == "tick") { unsigned ticks = 0; success = static_cast<bool>(input >> ticks) && farm.Tick(ticks); }
        else if (command == "harvest") { unsigned x = 0, z = 0, units = 0; success = static_cast<bool>(input >> x >> z) && farm.Harvest(static_cast<uint16_t>(x), static_cast<uint16_t>(z), units); if (success) std::cout << "harvested=" << units << '\n'; }
        else if (command == "plant") { unsigned x = 0, z = 0; std::string cropName; FarmCrop crop{}; success = static_cast<bool>(input >> x >> z >> cropName) && ParseCrop(cropName, crop) && farm.Plant(static_cast<uint16_t>(x), static_cast<uint16_t>(z), crop); }
        else if (command == "animal") { std::string animal; success = static_cast<bool>(input >> animal) && animal == "hen" && farm.AddAnimal(FarmAnimal::Hen); }
        else if (command == "sell") { unsigned long long id = 0; unsigned units = 0; long long price = 0; std::string itemName; FarmItem item{}; success = static_cast<bool>(input >> id >> itemName >> units >> price) && ParseProduce(itemName, item) && farm.Sell(id, item, units, price); }
        else { std::cout << "ERR unknown_command\n"; continue; }
        if (success) std::cout << "OK\n";
        else std::cout << "ERR " << ErrorName(farm.LastError()) << '\n';
    }
    std::cout << "FARM_DEMO_EXIT coins=" << farm.Coins() << " tick=" << farm.SimulationTick() << '\n';
    return 0;
}
