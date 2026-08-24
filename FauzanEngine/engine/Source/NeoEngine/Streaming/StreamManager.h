#pragma once
#include "StreamWorker.h"
#include <vector>
class StreamManager{
    StreamQueue queue;
    [[maybe_unused]] std::vector<StreamWorker*> workers;
public:
    StreamManager(int threads){
        for(int i=0;i<threads;i++) workers.push_back(new StreamWorker(&queue));
    }
    ~StreamManager(){
        for(auto w:workers){
            w->stop();
            delete w;
        }
    }
    void request(const StreamRequest& r){
        queue.push(r);
    }
};
