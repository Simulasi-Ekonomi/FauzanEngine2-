#include "NeoEditor.h"
#include "ActorCore.h"
#include <android/log.h>

void UNeoEditor::SelectActor(AActorCore* Target) {
    SelectedActor = Target;
    __android_log_print(ANDROID_LOG_INFO, "NeoEditor", "Selected: %s", Target->ActorName.c_str());
}

void UNeoEditor::TranslateActor(float X, float Y, float Z) {
    if(SelectedActor) {
        SelectedActor->Position[0] += X;
        SelectedActor->Position[1] += Y;
        SelectedActor->Position[2] += Z;
    }
}
