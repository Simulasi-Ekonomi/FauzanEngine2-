#pragma once

#include <vector>
#include <cassert>
#include "SparseSet.h"
#include "Entity.h"

namespace NeoEngine {

template<typename T>
class ComponentPool
{
public:

    void Insert(Entity e, const T& component)
    {
        sparse.Insert(e.index);

        if (sparse.Size() > components.size())
            components.push_back(component);
        else
            components[sparse.Index(e.index)] = component;
    }

    void Remove(Entity e)
    {
        if (!sparse.Contains(e.index))
            return;

        uint32_t index = sparse.Index(e.index);
        uint32_t lastEntity = sparse.GetDense(sparse.Size() - 1);

        components[index] = components.back();
        components.pop_back();

        sparse.Remove(e.index);
    }

    T& Get(Entity e)
    {
        assert(sparse.Contains(e.index));
        return components[sparse.Index(e.index)];
    }

    bool Has(Entity e)
    {
        return sparse.Contains(e.index);
    }

    size_t Size() const
    {
        return sparse.Size();
    }

private:

    SparseSet sparse;
    [[maybe_unused]] std::vector<T> components;
};

}
