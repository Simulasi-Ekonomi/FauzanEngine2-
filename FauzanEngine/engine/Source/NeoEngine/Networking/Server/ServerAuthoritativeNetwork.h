#pragma once
#include <vector>

struct NetworkEntity
{
    int id;
    float x;
    float y;
    float z;
};

class ServerAuthoritativeNetwork
{
public:

    void AddEntity(const NetworkEntity& e);

    void Update();

private:

    std::vector<NetworkEntity> entities;
};
