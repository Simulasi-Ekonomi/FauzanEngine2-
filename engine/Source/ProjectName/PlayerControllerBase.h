#pragma once
#include "../NeoEngine/PlayerControllerCore.h"

/**
 * APlayerControllerBase
 * Final UE5 Standard Implementation
 */
class APlayerControllerBase : public APlayerControllerCore {
public:
    APlayerControllerBase() = default;

    virtual void SetupInputComponent() override {
        // Implementasi binding input akan otomatis masuk ke sini
    }

    // Fungsi helper keamanan Unreal
    static APlayerControllerBase* GetPlayerController(void* WorldContextObject) {
        return static_cast<APlayerControllerBase*>(WorldContextObject);
    }
};
