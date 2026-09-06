// EOF
#ifndef NEO_AUTHORING_CATALOG_HPP
#define NEO_AUTHORING_CATALOG_HPP

#include <cstdint>

namespace NeoEngine {

struct BoneData {
    uint32_t lengthMillimeters;
};

class AuthoringCatalog {
public:
    bool ValidateBone(const BoneData& bone) const noexcept {
        if (bone.lengthMillimeters == 0 || bone.lengthMillimeters > 100000U) {
            return false;
        }
        return true;
    }
};

} // namespace NeoEngine

#endif // NEO_AUTHORING_CATALOG_HPP
