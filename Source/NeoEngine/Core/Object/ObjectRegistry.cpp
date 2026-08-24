#include "ObjectRegistry.h"

ObjectRegistry& ObjectRegistry::Get() {
    static ObjectRegistry Instance;
    return Instance;
}

ObjectID ObjectRegistry::Register(ObjectBase* Obj) {
    if (!Obj) return INVALID_OBJECT_ID;
    
    ObjectID NewId = NextID++;
    Obj->ID = NewId;
    ObjectsMap[NewId] = Obj;
    Obj->OnCreate();
    return NewId;
}

void ObjectRegistry::Unregister(ObjectID Id) {
    auto It = ObjectsMap.find(Id);
    if (It != ObjectsMap.end()) {
        It->second->OnDestroy();
        ObjectsMap.erase(It);
    }
}

ObjectBase* ObjectRegistry::GetObject(ObjectID Id) {
    auto It = ObjectsMap.find(Id);
    return (It != ObjectsMap.end()) ? It->second : nullptr;
}
