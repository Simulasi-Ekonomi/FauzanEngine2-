#pragma once
#include <vector>
#include <string>

struct CloudNode
{
    std::string address;
    int load;
};

class CloudServerManager
{
public:

    void AddNode(const CloudNode& node);
    void BalanceLoad();

private:

    std::vector<CloudNode> nodes;
};
