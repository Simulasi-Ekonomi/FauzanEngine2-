#pragma once
#include <chrono>
#include <thread>
class FramePacer{
    double target;
public:
    FramePacer(double fps){
        target = 1.0 / fps;
    }
    template<typename Clock>
    void regulate(typename Clock::time_point start){
        using namespace std::chrono;
        auto now = Clock::now();
        auto elapsed = duration<double>(now - start).count();
        if(elapsed < target){
            std::this_thread::sleep_for(
                duration<double>(target - elapsed));
        }
    }
};
