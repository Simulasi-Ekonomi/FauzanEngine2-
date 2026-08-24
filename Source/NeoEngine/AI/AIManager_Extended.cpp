#include "AIManager.h"
#include <iostream>

FAIManager& FAIManager::Get()
{
    static FAIManager Instance;
    return Instance;
}

void FAIManager::Update()
{
    std::cout << "[AI] Updating brain..." << std::endl;
}
