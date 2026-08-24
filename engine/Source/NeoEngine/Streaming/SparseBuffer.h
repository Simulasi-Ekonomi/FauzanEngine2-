#pragma once
#include <unordered_map>
class SparseBuffer{
    std::unordered_map<size_t,bool> pages;
public:
    void commit(size_t page){
        pages[page]=true;
    }
    bool resident(size_t page){
        return pages.count(page) > 0;
    }
};
