#pragma once
#include <typeindex>
#include <unordered_map>
#include <memory>

namespace NeoEngine {

class IComponent {
public:
    virtual ~IComponent() = default;
};

using ComponentType = std::type_index;
using ComponentPtr = std::shared_ptr<IComponent>;

} // namespace NeoEngine
