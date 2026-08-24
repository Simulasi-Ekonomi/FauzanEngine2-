#include "CharacterBase.h"
#include "../NeoEngine/ActorCore.h"
#include <android/log.h>

void InitializeGenesisScene() {
    __android_log_print(ANDROID_LOG_INFO, "Sovereign_Log", "🔱 Initializing Genesis Scene...");

    // 1. Spawn Lantai (Floor Actor)
    AActorCore* Floor = new AActorCore();
    Floor->ActorName = "Ground_Plane";
    Floor->Position[1] = -1.0f; // Berada di bawah karakter
    Floor->Scale[0] = 100.0f; Floor->Scale[2] = 100.0f; 

    // 2. Spawn Karakter Utama
    ACharacterBase* Player = new ACharacterBase();
    Player->ActorName = "Neo_Sovereign_Alpha";
    Player->Position[1] = 0.0f; // Berdiri di atas lantai
    Player->BeginPlay();

    __android_log_print(ANDROID_LOG_INFO, "Sovereign_Log", "✅ Scene Ready: Actors Spawned.");
}
