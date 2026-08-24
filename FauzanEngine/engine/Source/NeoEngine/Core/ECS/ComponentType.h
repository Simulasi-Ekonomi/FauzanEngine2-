#pragma once
#include <cstddef>

namespace NeoEngine
{

inline size_t GetUniqueComponentTypeID()
{
    static size_t lastID = 0;
    return lastID++;
}

template<typename T>
inline size_t GetComponentTypeID()
{
    static size_t typeID = GetUniqueComponentTypeID();
    return typeID;
}

}
