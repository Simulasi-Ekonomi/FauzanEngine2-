#pragma once
#include "ObjectID.h"
#include "ReflectionMacros.h"

class ObjectBase {
public:
    ObjectID ID;

    ObjectBase() : ID(0) {}
    virtual ~ObjectBase() = default;

    // Kontrak Refleksi
    static TypeInfo StaticType;
    virtual const TypeInfo* GetType() const { return &ObjectBase::StaticType; }

    virtual void OnCreate() {}
    virtual void OnDestroy() {}

    template<typename T>
    bool IsA() const {
        return GetType()->IsChildOf(&T::StaticType);
    }
};
