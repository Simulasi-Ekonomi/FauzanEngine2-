#pragma once
#include "ActorCore.h"

class UNeoEditor {
public:
    AActorCore* SelectedActor = nullptr;
    void SelectActor(AActorCore* Target);
    void TranslateActor(float X, float Y, float Z);
    void InitEditorUI();
};
