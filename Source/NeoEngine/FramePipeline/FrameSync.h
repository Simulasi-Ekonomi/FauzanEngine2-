#pragma once
#include <atomic>
class FrameSync{
    std::atomic<uint64_t> gpuDone{0};
public:
    void signalGPU(uint64_t frame){
        gpuDone.store(frame,std::memory_order_release);
    }
    void waitCPU(uint64_t frame){
        while(gpuDone.load(std::memory_order_acquire) < frame){
            // Spin-lock ringan untuk latensi minimal
        }
    }
};
