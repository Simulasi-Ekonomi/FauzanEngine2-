#include "Systems/GridNavigation.h"

#include <cstdio>
#include <vector>

int main() {
    using namespace NeoEngine;
    GridNavigation navigation, uninitialized;
    std::vector<GridCell> route{{7, 7}};
    if (uninitialized.FindPath({0, 0}, {1, 1}, route) || uninitialized.LastError() != GridNavigationError::NotInitialized || route != std::vector<GridCell>{{7, 7}}) return 1;
    if (!navigation.Initialize(8)) return 1;
    for (uint16_t z = 0; z < 7; ++z) if (!navigation.SetBlocked({3, z}, true)) return 1;
    if (!navigation.FindPath({1, 1}, {6, 1}, route) || route.size() <= 5 || route.front() != GridCell{1, 1} || route.back() != GridCell{6, 1}) return 1;
    for (const GridCell cell : route) if (navigation.IsBlocked(cell)) return 1;
    const std::vector<GridCell> preserved = route;
    if (navigation.FindPath({8, 1}, {6, 1}, route) || navigation.LastError() != GridNavigationError::OutOfBounds || route != preserved) return 1;
    if (navigation.FindPath({3, 1}, {6, 1}, route) || navigation.LastError() != GridNavigationError::BlockedEndpoint || route != preserved) return 1;
    for (uint16_t z = 0; z < 8; ++z) if (!navigation.SetBlocked({4, z}, true)) return 1;
    if (navigation.FindPath({1, 1}, {6, 1}, route) || navigation.LastError() != GridNavigationError::Unreachable || route != preserved) return 1;
    std::printf("GRID_NAVIGATION_SMOKE_OK side=8 detour=1 blocked=1 unreachable=1 atomic=1\n");
    return 0;
}
