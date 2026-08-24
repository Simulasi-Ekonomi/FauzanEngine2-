#pragma once
#include <string>
class APlayerStateCore {
public:
    std::string SovereignPlayerName;
    int PlayerID;
    void SetPlayerName(std::string InName);
};