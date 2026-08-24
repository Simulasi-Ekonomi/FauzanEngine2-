#include <android/log.h>
#include <chrono>
#include <thread>
#include <iostream>

#include "../Core/Engine.h"
#include "../Core/Config.h"
#include "../Core/Memory/MemoryManager.h"
#include "../Core/Debug/Stats.h"
#include "../Core/Debug/MemoryTracker.h"
#include "../World/NeoWorld.h"
#include "../ECS/EntityManager.h"
#include "../Systems/RPGSystem.h"
#include "../Systems/CombatSystem.h"
#include "../Systems/FarmingSystem.h"
#include "../Systems/MarketplaceSystem.h"
#include "../Systems/ItemSerialTracker.h"
#include "../Systems/AntiCheatSystem.h"
#include "../Systems/FraudDetectionSystem.h"
#include "../Systems/GameAnalyticsSystem.h"
#include "../API/PrivateAPISystem.h"
#include "../AI/AIManager.h"

#define LOG_TAG "EngineTest"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

using namespace NeoEngine;

int main() {
    LOGI("========================================");
    LOGI("  FAUZANENGINE RUNTIME TEST HARNESS");
    LOGI("========================================");

    // ============ FASE 1: INISIALISASI ENGINE ============
    LOGI("[1/5] Initializing Engine Core...");
    
    // Init memory
    MemoryManager::Init();
    LOGI("  Memory Manager: OK (%zu bytes allocated)", MemoryManager::GetTotalAllocated());
    
    // Init engine
    Engine::Get().Init();
    LOGI("  Engine: OK (FPS target: %d)", Engine::Get().GetTargetFPS());

    // ============ FASE 2: INISIALISASI SISTEM ============
    LOGI("[2/5] Initializing Game Systems...");
    
    // World
    NeoWorld world;
    LOGI("  NeoWorld: Ready");
    
    // ECS
    EntityManager entityMgr;
    LOGI("  ECS: Ready (living entities: %zu)", entityMgr.GetLivingEntityCount());
    
    // RPG System
    RPGSystem rpg;
    auto* player = rpg.CreateCharacter("player_1", "NeoHero");
    rpg.AddExp(player, 250);  // Level up to 2
    LOGI("  RPG: %s Level %d, HP %d/%d", player->name.c_str(), player->level, player->hp, player->maxHp);
    
    // Combat System
    CombatSystem combat;
    CombatStats attacker{100, 100, 20, 10, 1.5f, 2.0f, 0.15f, 2.0f, 0};
    CombatStats defender{80, 80, 15, 12, 1.0f, 2.0f, 0.1f, 1.5f, 0};
    DamageInfo dmg = combat.CalculateDamage(attacker, defender);
    LOGI("  Combat: Damage=%f, Critical=%d", dmg.amount, dmg.critical);
    
    // Farming System
    FarmingSystem farm;
    farm.TillPlot(3, 3);
    farm.PlantCrop(3, 3, CropType::Wheat);
    farm.WaterPlot(3, 3);
    LOGI("  Farming: Plot tilled & planted");
    
    // Serial Tracker
    ItemSerialTracker serialTracker;
    serialTracker.RegisterItem("player_1", "NeoHero", "weapon", "Excalibur", 1, "quest_reward");
    LOGI("  SerialTracker: Items=%d", serialTracker.GetTotalItems());
    
    // Marketplace
    MarketplaceSystem market;
    market.SetSerialTracker(&serialTracker);
    auto* listing = market.CreateListing("player_1", "NeoHero", "FE-WP-0001", "Excalibur", "weapon", TradeCurrency::GOLD, 5000);
    LOGI("  Marketplace: Listing created (%s)", listing ? listing->listingId.c_str() : "FAILED");
    
    // AntiCheat
    AntiCheatSystem antiCheat;
    antiCheat.SetSerialTracker(&serialTracker);
    antiCheat.DetectAndPunish("hacker_1", "CheaterBot", "speed_hack", "", "Moving too fast");
    LOGI("  AntiCheat: Hacker banned");
    
    // Fraud Detection
    FraudDetectionSystem fraud;
    bool valid = fraud.ValidateTransaction("buyer_1", "seller_1", "FE-WP-0002", 10000);
    LOGI("  FraudDetection: Transaction valid=%d", valid);
    
    // Analytics
    GameAnalyticsSystem analytics;
    analytics.TrackEvent("test_event", "testing", {{"status", "running"}});
    LOGI("  Analytics: Events=%d", analytics.GetEventCount());
    
    // Private API
    PrivateAPISystem api;
    api.SetMasterSecretKey("test-master-key-2024");
    LOGI("  PrivateAPI: Sessions=%d", api.GetActiveSessions());
    
    // AI Manager
    AIManager& ai = AIManager::GetInstance();
    LOGI("  AI: Status=%s", ai.GetStatusReport().c_str());

    // ============ FASE 3: SPAWN WORLD ACTORS ============
    LOGI("[3/5] Spawning World Actors...");
    
    EntityID tree1 = world.SpawnActor("Oak_Tree", "tree", 10, 0, 20);
    EntityID rock1 = world.SpawnActor("Granite_Boulder", "rock", -5, 0, 15);
    EntityID npc1 = world.SpawnActor("Guard_Captain", "npc", 0, 0, 0);
    LOGI("  World: %zu actors (Tree=%u, Rock=%u, NPC=%u)", world.GetActorCount(), tree1, rock1, npc1);

    // ============ FASE 4: SIMULASI GAME LOOP ============
    LOGI("[4/5] Running Simulation Loop (5 detik)...");
    
    float simTime = 0;
    int frames = 0;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    while (simTime < 5.0f) {
        float dt = 0.016f;  // 60 FPS
        
        // Update systems
        Engine::Get().Tick();
        world.Update(dt);
        farm.Update(dt);
        analytics.Update(dt);
        
        simTime += dt;
        frames++;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    float elapsed = std::chrono::duration<float>(endTime - startTime).count();
    float avgFPS = frames / elapsed;
    
    LOGI("  Simulation: %d frames in %.2fs (%.1f FPS avg)", frames, elapsed, avgFPS);

    // ============ FASE 5: LAPORAN AKHIR ============
    LOGI("[5/5] Final Report:");
    LOGI("  Memory: %zu bytes allocated", MemoryManager::GetTotalAllocated());
    LOGI("  World Actors: %zu", world.GetActorCount());
    LOGI("  ECS Entities: %zu", entityMgr.GetLivingEntityCount());
    LOGI("  Items Tracked: %d", serialTracker.GetTotalItems());
    LOGI("  Marketplace Listings: %zu", market.GetActiveListings().size());
    
    // Shutdown
    Engine::Get().Shutdown();
    MemoryManager::Shutdown();
    
    LOGI("========================================");
    LOGI("  TEST HARNESS COMPLETE - ENGINE OK!");
    LOGI("========================================");
    
    return 0;
}
