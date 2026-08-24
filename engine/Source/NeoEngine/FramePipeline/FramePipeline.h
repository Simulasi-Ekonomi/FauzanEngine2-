#pragma once
#include "FrameQueue.h"
#include "FrameContext.h"
#include "FrameSync.h"
#include "FramePacer.h"
#include "RenderAhead.h"
#include <chrono>
#include <functional>

class FramePipeline{
    FrameQueue<FrameContext> frames;
    FrameSync sync;
    RenderAhead ahead;
    FramePacer pacer;
public:
    FramePipeline(int buffered, double fps)
        : frames(buffered), pacer(fps){}

    template<typename CPU, typename GPU>
    void run(CPU cpu, GPU gpu){
        using Clock = std::chrono::high_resolution_clock;
        while(true){
            auto start = Clock::now();
            if(!ahead.canBuild()){
                std::this_thread::yield();
                continue;
            }

            auto& frame = frames.next();
            frame.frameIndex++;

            // Tunggu GPU selesai dengan buffer ini jika perlu
            if(frame.frameIndex > frames.size()) {
                sync.waitCPU(frame.frameIndex - frames.size());
            }

            cpu(frame);
            ahead.cpuProduced();

            gpu(frame, [this, &frame]{
                sync.signalGPU(frame.frameIndex);
                ahead.gpuFinished();
            });

            pacer.regulate<Clock>(start);
        }
    }
};
