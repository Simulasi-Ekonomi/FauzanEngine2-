#pragma once
#include "../NeoEngine/GameInstanceCore.h"
class UGameInstanceBase : public UGameInstanceCore {
public:
    virtual void Init() override;
    virtual void Shutdown() override;
};
