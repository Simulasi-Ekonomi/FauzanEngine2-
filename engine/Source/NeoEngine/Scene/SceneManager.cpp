#include "SceneManager.h"
#include "../ActorCore.h"

void SceneManager::SpawnActor(AActor* NewActor, Vector3 Location, Rotator Rotation) {
    if (NewActor) {
        NewActor->SetActorLocation(Location);
        NewActor->SetActorRotation(Rotation);
        ActiveActors.push_back(NewActor);
        NewActor->BeginPlay();
    }
}

void SceneManager::UpdateScene(float DeltaTime) {
    for (AActor* Actor : ActiveActors) {
        if (Actor && !Actor->IsPendingKill()) {
            Actor->Tick(DeltaTime);
        }
    }
}
