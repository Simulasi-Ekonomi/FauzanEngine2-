#include "GameModeCore.h"
#include <android/log.h>

void AGameModeCore::StartGame() {
    __android_log_print(ANDROID_LOG_INFO, "NeoEngine", "Sovereign Game Mode: Match Started.");
    // Logic untuk spawning player dan reset score
}

void AGameModeCore::PostLogin() {
    __android_log_print(ANDROID_LOG_INFO, "NeoEngine", "Player joined the session.");
}