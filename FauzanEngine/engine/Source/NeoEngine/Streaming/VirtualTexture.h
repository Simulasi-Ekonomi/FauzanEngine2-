#pragma once
#include <unordered_map>
struct Tile{ int x,y,mip; };
class VirtualTexture{
    std::unordered_map<long long,bool> loaded;
    long long key(const Tile& t) const{
        return ((long long)t.x<<32) | (t.y<<16) | t.mip;
    }
public:
    bool resident(const Tile& t){
        return loaded.count(key(t)) > 0;
    }
    void markLoaded(const Tile& t){
        loaded[key(t)] = true;
    }
};
