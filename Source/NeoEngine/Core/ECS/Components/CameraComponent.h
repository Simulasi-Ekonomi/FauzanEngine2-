#pragma once

namespace NeoEngine
{

struct CameraComponent
{
    float fov        = 60.0f;
    float aspect     = 800.0f / 600.0f;
    float nearPlane  = 0.1f;
    float farPlane   = 100.0f;
};

}
