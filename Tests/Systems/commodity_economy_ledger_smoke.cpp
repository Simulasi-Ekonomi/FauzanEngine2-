#include "Systems/CommodityEconomyLedger.h"

#include <cstdio>

int main() {
    using namespace NeoEngine; CommodityCatalog catalog; CommodityEconomyLedger ledger;
    if(!catalog.Add({"wheat",CommodityCategory::Crop,5,20,true})||!catalog.Add({"artifact",CommodityCategory::QuestGood,100,2,false})||!ledger.Apply(catalog,{"harvest-1",CommodityEconomyKind::HarvestGrant,"wheat",10})||ledger.Quantity("wheat")!=10U||!ledger.Apply(catalog,{"sell-1",CommodityEconomyKind::ShopSell,"wheat",4})||ledger.Coins()!=20||ledger.Quantity("wheat")!=6U||!ledger.Apply(catalog,{"buy-1",CommodityEconomyKind::ShopBuy,"wheat",2})||ledger.Coins()!=10||ledger.Quantity("wheat")!=8U) return 1;
    const int64_t coins=ledger.Coins();const uint16_t quantity=ledger.Quantity("wheat");if(!ledger.Apply(catalog,{"sell-1",CommodityEconomyKind::ShopSell,"wheat",4})||ledger.Coins()!=coins||ledger.Quantity("wheat")!=quantity||ledger.Apply(catalog,{"buy-2",CommodityEconomyKind::ShopBuy,"wheat",3})||ledger.LastError()!=CommodityEconomyError::InsufficientCoins||ledger.Apply(catalog,{"artifact-1",CommodityEconomyKind::ShopBuy,"artifact",1})||ledger.LastError()!=CommodityEconomyError::InvalidCommand||ledger.Apply(catalog,{"bad",CommodityEconomyKind::HarvestGrant,"missing",1})||ledger.LastError()!=CommodityEconomyError::UnknownCommodity) return 1;
    std::vector<uint8_t> bytes;if(!ledger.Serialize(bytes)) return 1;CommodityEconomyLedger restored;if(!restored.Deserialize(catalog,bytes)||restored.Coins()!=coins||restored.Quantity("wheat")!=quantity||!restored.HasApplied("sell-1")) return 1;bytes.back()^=0x01U;if(restored.Deserialize(catalog,bytes)||restored.LastError()!=CommodityEconomyError::ChecksumMismatch||restored.Coins()!=coins||restored.Quantity("wheat")!=quantity) return 1;
    std::printf("COMMODITY_ECONOMY_LEDGER_SMOKE_OK harvest=1 shop=1 idempotent=1 bounded=1 persistence=1 coins=%lld\n",static_cast<long long>(ledger.Coins()));return 0;
}
