#include "Systems/FarmSystem.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    FarmBalanceProfile invalid{};
    invalid.maxEnergy = 0U;
    FarmSystem rejected(2U, 2U, 10, invalid);
    if (rejected.IsReady() || rejected.LastError() != FarmError::InvalidConfiguration) return 1;

    FarmBalanceProfile balance{};
    balance.maxEnergy = 100U;
    balance.energyRegenPerTick = 0U;
    balance.tillEnergyCost = 1U;
    balance.plantEnergyCost = 1U;
    balance.waterEnergyCost = 1U;
    balance.harvestEnergyCost = 1U;
    balance.growthTicks = {1U, 2U, 3U};
    balance.harvestYield = {4U, 5U, 6U};
    balance.sellPrice = {7LL, 8LL, 9LL};
    FarmSystem farm(3U, 1U, 10, balance);
    if (!farm.IsReady() || farm.Balance().growthTicks != balance.growthTicks || farm.Energy() != 100U) return 2;

    const FarmCrop crops[] = {FarmCrop::Wheat, FarmCrop::Corn, FarmCrop::Tomato};
    const uint32_t expectedYield[] = {4U, 5U, 6U};
    const int64_t expectedPrice[] = {7LL, 8LL, 9LL};
    int64_t expectedCoins = 10;
    for (uint16_t index = 0U; index < 3U; ++index) {
        if (!farm.Till(index, 0U) || !farm.Plant(index, 0U, crops[index]) || !farm.Water(index, 0U)) return 3;
        if (!farm.Tick(balance.growthTicks[index] == 0U ? 1U : balance.growthTicks[index])) return 4;
        uint32_t harvested = 0U;
        if (!farm.Harvest(index, 0U, harvested) || harvested != expectedYield[index]) return 5;
        if (!farm.SellCrop(100U + index, crops[index], harvested)) return 6;
        expectedCoins += static_cast<int64_t>(expectedYield[index]) * expectedPrice[index];
        if (farm.Coins() != expectedCoins) return 7;
    }
    if (farm.Energy() != 88U || farm.LastError() != FarmError::None) return 8;
    std::printf("FARM_BALANCE_PROFILE_SMOKE_OK invalid_rejected=1 crops=3 growth=authored yield=authored price=authored energy=88\n");
    return 0;
}
