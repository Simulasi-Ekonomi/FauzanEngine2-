#include "ServerAuthoritativeNetwork.h"
#include <iostream>

void ServerAuthoritativeNetwork::AddEntity(const NetworkEntity& e)
{
    entities.push_back(e);
}

void ServerAuthoritativeNetwork::Update()
{
    for(const auto& e : entities)
    {
        std::cout<<"Server sync entity "<<e.id<<"\n";
    }
}
