#pragma once

#include <cstddef>
#include "SparseSet.h"
#include "Entity.h"

namespace NeoEngine {

template<typename T>
class ComponentPool
{
public:

    void Insert(Entity e, const T& component)
    {
        components_.Insert(e, component);
    }

    void Remove(Entity e)
    {
        components_.Remove(e);
    }

    T& Get(Entity e)
    {
        return components_.Get(e);
    }

    const T& Get(Entity e) const
    {
        return components_.Get(e);
    }

    bool Has(Entity e) const
    {
        return components_.Has(e);
    }

    size_t Size() const
    {
        return components_.Size();
    }

private:

    SparseSet<T> components_;
};

}
