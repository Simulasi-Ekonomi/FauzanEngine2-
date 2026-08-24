#pragma once
#include <vector>

struct OcclusionObject
{
    float x;
    float y;
    float z;
    float size;
};

class GPUOcclusionCulling
{
public:

    void AddObject(const OcclusionObject& obj);

    void PerformCulling();

    const std::vector<int>& VisibleObjects() const;

private:

    std::vector<OcclusionObject> objects;
    std::vector<int> visible;
};
