#pragma once
#include <unordered_set>
class Residency{
    std::unordered_set<size_t> active;
public:
    void touch(size_t id){
        active.insert(id);
    }
    bool used(size_t id){
        return active.count(id) > 0;
    }
    void clear(){
        active.clear();
    }
};
