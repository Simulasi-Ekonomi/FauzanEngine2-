#include "Runtime/LodManager.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

int main() {
    using namespace NeoEngine;

    LodManager manager;
    std::vector<MeshVariant> variants(4);
    variants[0].vertexCount = 1000;
    variants[1].vertexCount = 500;
    variants[2].vertexCount = 250;
    variants[3].vertexCount = 100;

    uint8_t selectedLod = 99;
    assert(manager.Register("mesh", variants));
    assert(manager.SelectLod("mesh", 50.f, selectedLod) && selectedLod == 0);
    assert(manager.SelectLod("mesh", 100.f, selectedLod) && selectedLod == 0);
    assert(manager.SelectLod("mesh", 200.f, selectedLod) && selectedLod == 1);
    assert(manager.SelectLod("mesh", 500.f, selectedLod) && selectedLod == 1);
    assert(manager.SelectLod("mesh", 1000.f, selectedLod) && selectedLod == 2);
    assert(manager.SelectLod("mesh", 2000.f, selectedLod) && selectedLod == 2);
    assert(manager.SelectLod("mesh", 5000.f, selectedLod) && selectedLod == 3);

    assert(manager.GetVariant("mesh", 3) != nullptr);
    assert(manager.GetVariant("mesh", 4) == nullptr);
    assert(manager.LastError());

    uint32_t mip = 99;
    assert(manager.SelectTextureMip(2048, 0.f, mip) && mip == 0);
    assert(manager.SelectTextureMip(2048, 1000.f, mip) && mip == 4);
    assert(manager.SelectTextureMip(2048, 5000.f, mip) && mip == 8);

    assert(!manager.Register("", variants));
    assert(!manager.Register("empty", {}));
    assert(!manager.Register("too-many", std::vector<MeshVariant>(5)));

    selectedLod = 77;
    assert(!manager.SelectLod("missing", 10.f, selectedLod));
    assert(selectedLod == 77);
    assert(!manager.SelectLod("mesh", -1.f, selectedLod));
    assert(!manager.SelectLod("mesh", std::numeric_limits<float>::quiet_NaN(), selectedLod));
    assert(!manager.SelectLod("mesh", std::numeric_limits<float>::infinity(), selectedLod));

    mip = 77;
    assert(!manager.SelectTextureMip(0, 10.f, mip));
    assert(mip == 77);
    assert(!manager.SelectTextureMip(2048, -1.f, mip));
    assert(!manager.SelectTextureMip(2048, std::numeric_limits<float>::quiet_NaN(), mip));
    assert(!manager.SelectTextureMip(2048, std::numeric_limits<float>::infinity(), mip));

    return 0;
}
