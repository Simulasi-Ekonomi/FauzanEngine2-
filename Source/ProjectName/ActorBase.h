#pragma once
#include "../NeoEngine/ActorCore.h"

/**
 * AActorBase
 * Mengikuti standar Unreal Engine 5 Actor Lifecycle
 */
class AActorBase : public AActorCore {
public:
    AActorBase() {
        bAllowTickBeforeBeginPlay = false;
        bHasInitialized = false;
    }

    virtual void BeginPlay() override {
        if (!bHasInitialized) {
            AActorCore::BeginPlay();
            bHasInitialized = true;
        }
    }

    virtual void Tick(float DeltaTime) override {
        if (bHasInitialized) {
            AActorCore::Tick(DeltaTime);
        }
    }

protected:
    bool bAllowTickBeforeBeginPlay;
    bool bHasInitialized;
};
