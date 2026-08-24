#include "PlayerControllerCore.h"
#include <android/log.h>

void APlayerControllerCore::Possess(ACharacterCore* InCharacter) {
    if(InCharacter) {
        __android_log_print(ANDROID_LOG_INFO, "NeoEngine", "Possessing Character: %s", InCharacter->ActorName.c_str());
    }
}

void APlayerControllerCore::UpdateCamera(float DeltaTime) {
    // Logic untuk kalkulasi rotasi kamera berdasarkan input mouse/touch
}