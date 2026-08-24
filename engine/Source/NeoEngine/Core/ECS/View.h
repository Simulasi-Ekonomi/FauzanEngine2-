#pragma once

#include <vector>
#include "Entity.h"

namespace NeoEngine {

class Registry;

template<typename... Components>
class View
{
public:

    View(Registry& reg) : registry(reg) {}

    std::vector<Entity> Get();

private:

    Registry& registry;
};

} // namespace NeoEngine
