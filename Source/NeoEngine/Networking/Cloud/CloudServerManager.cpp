#include "CloudServerManager.h"
#include <iostream>

void CloudServerManager::AddNode(const CloudNode& node)
{
    nodes.push_back(node);
}

void CloudServerManager::BalanceLoad()
{
    for(const auto& n : nodes)
    {
        std::cout<<"Cloud node "<<n.address<<" load "<<n.load<<"\n";
    }
}
