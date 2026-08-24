#include "ActorCore.h"
#include <android/log.h>

void AActorCore::BeginPlay() {
    __android_log_print(ANDROID_LOG_INFO, "NeoEngine", "Actor %s spawned.", ActorName.c_str());
}

void AActorCore::Tick(float DeltaTime) {
    // Base tick logic (Physics, Collision checks)
}