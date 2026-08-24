#pragma once
#include <vector>
template<typename T>
class FrameQueue{
    [[maybe_unused]] std::vector<T> frames;
    size_t head = 0;
public:
    FrameQueue(size_t count):frames(count){}
    T& current(){ return frames[head]; }
    T& next(){
        head = (head + 1) % frames.size();
        return frames[head];
    }
    size_t size() const{ return frames.size(); }
};
