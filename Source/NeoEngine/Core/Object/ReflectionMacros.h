#pragma once
#include "TypeInfo.h"

// Makro untuk menyuntikkan kemampuan refleksi ke dalam kelas
#define NEO_CLASS(type, parent) \
public: \
    static TypeInfo StaticType; \
    virtual const TypeInfo* GetType() const override { return &type::StaticType; } \
    using Super = parent;

#define NEO_CLASS_IMPLEMENT(type, parent_class) \
    TypeInfo type::StaticType = { \
        #type, \
        &parent_class::StaticType, \
        sizeof(type) \
    };

// Khusus untuk ObjectBase yang tidak punya parent engine
#define NEO_CLASS_ROOT_IMPLEMENT(type) \
    TypeInfo type::StaticType = { \
        #type, \
        nullptr, \
        sizeof(type) \
    };
