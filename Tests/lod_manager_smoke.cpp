#include <cassert>
#include <string>
#include <vector>
#include <unordered_map>

namespace NeoEngine {

using AssetID = std::string;

struct MeshVariant {
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
};

class LodManager {
public:
    [[nodiscard]] bool SelectLod(float cameraDistance, uint8_t& outLodLevel) const noexcept {
        // Simple distance-based LOD selection
        if (cameraDistance < 100.f) outLodLevel = 0;     // Full detail
        else if (cameraDistance < 500.f) outLodLevel = 1; // Medium
        else if (cameraDistance < 2000.f) outLodLevel = 2; // Low
        else outLodLevel = 3;                             // Minimal
        return true;
    }
};

} // namespace NeoEngine

int main() {
    NeoEngine::LodManager manager;
    uint8_t selectedLod = 0;

    // Test 1: LOD selection at different distances
    assert(manager.SelectLod(50.f, selectedLod) && selectedLod == 0);    // Closest = full detail
    assert(manager.SelectLod(200.f, selectedLod) && selectedLod == 1);   // Medium distance
    assert(manager.SelectLod(1000.f, selectedLod) && selectedLod == 2);  // Far
    assert(manager.SelectLod(5000.f, selectedLod) && selectedLod == 3);  // Very far = minimal

    // Test 2: Boundary cases
    assert(manager.SelectLod(100.f, selectedLod) && selectedLod == 0);
    assert(manager.SelectLod(500.f, selectedLod) && selectedLod == 1);
    assert(manager.SelectLod(2000.f, selectedLod) && selectedLod == 2);

    return 0;  // All tests passed
}
