#pragma once
#include <queue>
#include <mutex>
#include "StreamRequest.h"
class StreamQueue{
    std::queue<StreamRequest> q;
    std::mutex m;
public:
    void push(const StreamRequest& r){
        std::lock_guard<std::mutex> g(m);
        q.push(r);
    }
    bool pop(StreamRequest& r){
        std::lock_guard<std::mutex> g(m);
        if(q.empty()) return false;
        r=q.front();
        q.pop();
        return true;
    }
};
