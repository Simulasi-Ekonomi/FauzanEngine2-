#pragma once
#include "Core/ActorBase.h"

namespace NeoEngine {

class PlayerController : public ActorBase {
public:
    PlayerController() = default;
    virtual ~PlayerController() = default;

    void Possess(ActorBase* pawn);
    ActorBase* GetPawn() const { return m_Pawn; }

    void ProcessLookX(float value);
    void ProcessLookY(float value);

protected:
    ActorBase* m_Pawn = nullptr;
};

} // namespace NeoEngine
