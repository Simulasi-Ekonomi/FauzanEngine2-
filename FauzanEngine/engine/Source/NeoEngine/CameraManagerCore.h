#pragma once
class ACameraManagerCore {
public:
    float FOV = 90.0f;
    float ViewMatrix[16];
    void UpdateView(float Pos[3], float Rot[3]);
};