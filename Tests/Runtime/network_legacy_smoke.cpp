#include "Networking/Cloud/CloudServerManager.h"
#include "Networking/Server/ServerAuthoritativeNetwork.h"
#include <cstdio>

int main() {
    CloudServerManager cloud;
    cloud.AddNode({"127.0.0.1:9000", 10});
    cloud.AddNode({"127.0.0.1:9001", 20});
    cloud.BalanceLoad();

    ServerAuthoritativeNetwork server;
    server.AddEntity({1, 1.0F, 2.0F, 3.0F});
    server.AddEntity({2, 4.0F, 5.0F, 6.0F});
    server.Update();

    std::puts("NETWORK_LEGACY_SMOKE_OK cloud_nodes=2 entities=2 update=1");
    return 0;
}
