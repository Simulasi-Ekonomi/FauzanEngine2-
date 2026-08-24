#pragma once
#include <unordered_map>
#include "ObjectID.h"
#include "ObjectBase.h"

class ObjectRegistry {
public:
    static ObjectRegistry& Get();

    // Mendaftarkan objek ke dalam pengawasan engine
    ObjectID Register(ObjectBase* Obj);
    
    // Menghapus objek dari pengawasan
    void Unregister(ObjectID Id);
    
    // Mencari objek berdasarkan ID
    ObjectBase* GetObject(ObjectID Id);

    // Helper untuk pencarian dengan tipe spesifik
    template<typename T>
    T* GetObjectAs(ObjectID Id) {
        ObjectBase* Obj = GetObject(Id);
        return (Obj && Obj->IsA<T>()) ? static_cast<T*>(Obj) : nullptr;
    }

private:
    ObjectRegistry() = default;
    std::unordered_map<ObjectID, ObjectBase*> ObjectsMap;
    ObjectID NextID = 1;
};
