#pragma once

namespace NeoEngine
{

class ObjectBase;
class ObjectRegistry;

class GarbageCollector
{
public:
    static void Collect(ObjectRegistry& registry);
    static void Mark(ObjectBase* obj);
};

}
