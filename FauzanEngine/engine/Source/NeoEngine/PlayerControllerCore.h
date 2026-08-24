#pragma once
#include "CharacterCore.h"
class APlayerControllerCore {
public:
    void Possess(ACharacterCore* InCharacter);
    void UpdateCamera(float DeltaTime);
};