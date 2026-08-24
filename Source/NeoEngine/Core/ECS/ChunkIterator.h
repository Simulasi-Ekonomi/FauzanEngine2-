#pragma once
#include <vector>
#include <tuple>

template<typename... Components>
class ChunkIterator
{
public:

    ChunkIterator(std::vector<Components>&... comps)
        : components(comps...) {}

    template<typename Func>
    void ForEach(Func fn)
    {
        size_t size = std::get<0>(components).size();

        for(size_t i = 0; i < size; i++)
        {
            fn(std::get<std::vector<Components>>(components)[i]...);
        }
    }

private:

    std::tuple<std::vector<Components>&...> components;
};
