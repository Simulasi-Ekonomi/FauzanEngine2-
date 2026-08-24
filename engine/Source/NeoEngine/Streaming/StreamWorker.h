#pragma once
#include "StreamQueue.h"
#include <thread>
#include <atomic>
class StreamWorker{
    StreamQueue* queue;
    std::thread th;
    std::atomic<bool> running{true};
public:
    StreamWorker(StreamQueue* q):queue(q){
        th = std::thread([this]{loop();});
    }
    void loop(){
        StreamRequest r;
        while(running){
            if(queue->pop(r)){
                void* data = nullptr;
                // Di sini nanti tempat integrasi File I/O
                if(r.onLoaded) r.onLoaded(data);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }
    void stop(){
        running=false;
        if(th.joinable()) th.join();
    }
};
