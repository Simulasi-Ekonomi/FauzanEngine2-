#include "Systems/CommodityCatalog.h"

#include <cstdio>

int main() {
    using namespace NeoEngine; CommodityCatalog catalog;
    for(uint8_t index=0;index<50;++index) if(!catalog.Add({"commodity-"+std::to_string(index),static_cast<CommodityCategory>(index%6U),static_cast<int64_t>(index+1U)*10LL,static_cast<uint16_t>(index+1U),index%2U==0U})) { std::printf("COMMODITY_CATALOG_FAIL stage=add index=%u error=%u\n",static_cast<unsigned>(index),static_cast<unsigned>(catalog.LastError())); return 1; }
    std::vector<uint8_t> bytes;if(catalog.Count()!=50U||!catalog.Find("commodity-49")||catalog.Add({"commodity-49",CommodityCategory::Crop,1,1,true})||catalog.LastError()!=CommodityCatalogError::DuplicateId||!catalog.Serialize(bytes)) { std::printf("COMMODITY_CATALOG_FAIL stage=serialize error=%u\n",static_cast<unsigned>(catalog.LastError())); return 1; }
    CommodityCatalog restored;if(!restored.Add({"existing",CommodityCategory::Crop,1,1,true})||!restored.Deserialize(bytes)||restored.Count()!=50U||!restored.Find("commodity-0")) { std::printf("COMMODITY_CATALOG_FAIL stage=deserialize error=%u count=%zu\n",static_cast<unsigned>(restored.LastError()),restored.Count()); return 1; }
    bytes.back()^=0xFFU;if(restored.Deserialize(bytes)||restored.LastError()!=CommodityCatalogError::ChecksumMismatch||restored.Count()!=50U||catalog.Add({"bad id",CommodityCategory::Crop,1,1,true})||catalog.LastError()!=CommodityCatalogError::InvalidId) { std::printf("COMMODITY_CATALOG_FAIL stage=invalid error=%u count=%zu\n",static_cast<unsigned>(restored.LastError()),restored.Count()); return 1; }
    std::printf("COMMODITY_CATALOG_SMOKE_OK commodities=50 versioned=1 atomic=1 checksum=1\n");return 0;
}
