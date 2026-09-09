#pragma once
#include <vector>
#include <cstdint>
#include <cassert>
#include <limits>
#include <utility>
#include "Entity.h"

namespace NeoEngine {

template<typename T>
class SparseSet {

private:

    static constexpr uint32_t kInvalidIndex = std::numeric_limits<uint32_t>::max();
    std::vector<uint32_t> sparse;
    std::vector<Entity> dense;
    std::vector<T> components;

public:

    void Insert(Entity e, const T& component)
    {
        const uint32_t id = e.GetID();

        if (Has(e)) {
            components[sparse[id]] = component;
            return;
        }

        if (id >= sparse.size())
            sparse.resize(static_cast<size_t>(id) + 1U, kInvalidIndex);

        const uint32_t denseIndex = static_cast<uint32_t>(dense.size());
        dense.push_back(e);
        try {
            components.push_back(component);
        } catch (...) {
            dense.pop_back();
            throw;
        }
        sparse[id] = denseIndex;
    }

    bool Has(Entity e) const
    {
        const uint32_t id = e.GetID();

        if (id >= sparse.size())
            return false;

        const uint32_t idx = sparse[id];

        return idx < dense.size() && dense[idx] == e;
    }

    T* TryGet(Entity e)
    {
        if (!Has(e))
            return nullptr;
        return &components[sparse[e.GetID()]];
    }

    const T* TryGet(Entity e) const
    {
        if (!Has(e))
            return nullptr;
        return &components[sparse[e.GetID()]];
    }

    T& Get(Entity e)
    {
        T* component = TryGet(e);
        assert(component != nullptr);
        return *component;
    }

    const T& Get(Entity e) const
    {
        const T* component = TryGet(e);
        assert(component != nullptr);
        return *component;
    }

    void Remove(Entity e)
    {
        if (!Has(e))
            return;

        const uint32_t id = e.GetID();
        const uint32_t idx = sparse[id];
        const uint32_t last = static_cast<uint32_t>(dense.size() - 1U);

        if (idx != last) {
            dense[idx] = dense[last];
            components[idx] = std::move(components[last]);
            sparse[dense[idx].GetID()] = idx;
        }

        dense.pop_back();
        components.pop_back();
        sparse[id] = kInvalidIndex;
    }

    size_t Size() const
    {
        return components.size();
    }

};

}
