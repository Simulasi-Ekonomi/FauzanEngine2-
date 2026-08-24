#include "CharacterBase.h"
#include <android/log.h>

void ACharacterBase::BeginPlay() {
    // Memanggil fungsi dasar dari NeoEngine Core
    __android_log_print(ANDROID_LOG_INFO, "NeoProject", "Character Reborn: Ready for action.");
    WalkSpeed = 800.0f; // Kecepatan khusus untuk game ini
}

void ACharacterBase::SetupPlayerInputComponent() {
    // Menghubungkan tombol loncat ke fungsi Jump() di Core
}