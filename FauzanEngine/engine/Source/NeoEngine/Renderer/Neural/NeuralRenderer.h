#pragma once
#include <vector>

struct NeuralSample
{
    float x;
    float y;
    float z;
};

class NeuralRenderer
{
public:

    void AddSample(const NeuralSample& s);
    void TrainStep();

private:

    std::vector<NeuralSample> samples;
};
