#pragma once
class RenderAhead{
    int maxAhead = 2;
    int cpuFrame = 0;
    int gpuFrame = 0;
public:
    bool canBuild() const{
        return cpuFrame - gpuFrame < maxAhead;
    }
    void cpuProduced(){ cpuFrame++; }
    void gpuFinished(){ gpuFrame++; }
};
