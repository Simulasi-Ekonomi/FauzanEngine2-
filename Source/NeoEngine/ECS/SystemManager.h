#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include "EntityManager.h"

namespace NeoEngine {
class ISystem { public: virtual ~ISystem()=default; virtual void Update(float dt,EntityManager& em)=0; virtual const char* GetName()const=0; };
class SystemManager {
    std::vector<std::unique_ptr<ISystem>> m_Systems;
    std::unordered_map<std::string,ISystem*> m_NamedSystems;
public:
    template<typename T,typename... Args> T* AddSystem(Args&&... args) {
        auto s=std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr=s.get(); m_Systems.push_back(std::move(s)); m_NamedSystems[ptr->GetName()]=ptr; return ptr;
    }
    void UpdateAll(float dt,EntityManager& em) { for(auto& s:m_Systems) s->Update(dt,em); }
    ISystem* GetSystem(const std::string& name) { auto it=m_NamedSystems.find(name); return it!=m_NamedSystems.end()?it->second:nullptr; }
    void Clear() { m_Systems.clear(); m_NamedSystems.clear(); }
};
}
