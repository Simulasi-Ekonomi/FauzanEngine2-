#pragma once
#include "../NeoEngine/InputConfigCore.h"

class UInputConfig : public UInputConfigCore {
public:
    void SetupDefaults(); // Bind W,A,S,D secara otomatis
};