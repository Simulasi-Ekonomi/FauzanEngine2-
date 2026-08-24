#pragma once
#include <string>

class AActorCore {
public:
    float Position[3] = {0,0,0};
    float Rotation[3] = {0,0,0};
    float Scale[3] = {1,1,1};
    std::string ActorName = "Actor";

    virtual void BeginPlay();
    virtual void Tick(float DeltaTime);
};