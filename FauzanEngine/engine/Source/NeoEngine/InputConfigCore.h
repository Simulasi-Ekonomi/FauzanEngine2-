#pragma once
#include <map>
#include <string>

class UInputConfigCore {
public:
    void BindAction(std::string ActionName, int KeyCode);
    void ProcessInput(int KeyCode, bool bPressed);
private:
    std::map<std::string, int> ActionMappings;
};