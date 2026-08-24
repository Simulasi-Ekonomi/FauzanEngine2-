#pragma once
#include <string>
class AGameModeCore {
public:
    std::string DefaultPawnClass;
    virtual void PostLogin();
    void StartGame();
};