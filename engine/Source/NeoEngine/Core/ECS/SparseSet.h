#pragma once
#include <vector>
#include <cstdint>
#include <cassert>
#include "Entity.h"

namespace NeoEngine {

template<typename T>
class SparseSet {

private:

    std::vector<uint32_t> sparse;
    std::vector<Entity> dense;
    std::vector<T> components;

public:

    void Insert(Entity e, const T& component)
    {
        uint32_t id = e.index;

        if (id >= sparse.size())
            sparse.resize(id + 1, UINT32_MAX);

        sparse[id] = dense.size();

        dense.push_back(e);
        components.push_back(component);
    }

    bool Has(Entity e) const
    {
        uint32_t id = e.index;

        if (id >= sparse.size())
            return false;

        uint32_t idx = sparse[id];

        return idx < dense.size() && dense[idx] == e;
    }

    T& Get(Entity e)
    {
        uint32_t idx = sparse[e.index];
        return components[idx];
    }

    void Remove(Entity e)
    {
        uint32_t idx = sparse[e.index];
        uint32_t last = dense.size() - 1;

        dense[idx] = dense[last];
        components[idx] = components[last];

        sparse[dense[idx].index] = idx;

        dense.pop_back();
        components.pop_back();
    }

};

}
