#pragma once
#include "../NeoEngine/CharacterCore.h"
class ACharacterBase : public ACharacterCore {
public:
    virtual void BeginPlay() override;
    void SetupPlayerInputComponent();
};