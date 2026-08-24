#include "CharacterCore.h"
#include <android/log.h>

#define LOG_TAG "CharacterCore"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

ACharacterCore::ACharacterCore() {
    // Inisialisasi standar Unreal
}

ACharacterCore::~ACharacterCore() {
    // Cleanup
}

void ACharacterCore::MoveForward(float value) {
    // Placeholder forward vector (seharusnya dihitung dari rotasi)
    Vector3 forward(1.0f, 0.0f, 0.0f);
    Vector3 location = GetActorLocation();
    location = location + forward * value;
    SetActorLocation(location);
}

void ACharacterCore::Jump() {
    LOGI("%s jumped", GetName().c_str());
}

} // namespace NeoEngine
