#include "NeuralRenderer.h"

void NeuralRenderer::AddSample(const NeuralSample& s)
{
    samples.push_back(s);
}

void NeuralRenderer::TrainStep()
{
    for(auto& s : samples)
    {
        s.x *= 0.99f;
    }
}
