#pragma once

#include <vector>
#include <cstdint>

namespace NeoEngine {

template<typename T>
class LockFreeSparseSet
{
public:

    void Resize(size_t size)
    {
        sparse.resize(size, INVALID);
    }

    void Insert(uint32_t id, const T& value)
    {
        if (id >= sparse.size())
            Resize(id + 1);

        if (sparse[id] == INVALID)
        {
            sparse[id] = dense.size();
            dense.push_back(value);
        }
        else
        {
            dense[sparse[id]] = value;
        }
    }

    bool Has(uint32_t id) const
    {
        return id < sparse.size() && sparse[id] != INVALID;
    }

    T& Get(uint32_t id)
    {
        return dense[sparse[id]];
    }

    void Remove(uint32_t id)
    {
        if (!Has(id)) return;

        size_t index = sparse[id];
        dense[index] = dense.back();
        dense.pop_back();
        sparse[id] = INVALID;
    }

private:

    static constexpr uint32_t INVALID = UINT32_MAX;

    std::vector<uint32_t> sparse;
    std::vector<T> dense;
};

} // namespace NeoEngine
