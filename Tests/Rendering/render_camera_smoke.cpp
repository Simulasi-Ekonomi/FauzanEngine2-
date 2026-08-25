#include "Runtime/RenderCamera.h"

#include <cmath>
#include <cstdio>

int main() {
    using namespace NeoEngine; RenderCamera camera; RenderPoint3 clip{};
    if (camera.Initialize({RenderCameraMode::Orthographic, {}, 0.0F, 60.0F, 1.0F, 0.1F, 100.0F}) || camera.LastError() != RenderCameraError::InvalidConfiguration) return 1;
    if (!camera.Initialize({RenderCameraMode::Orthographic, {}, 5.0F, 60.0F, 2.0F, 0.1F, 100.0F}) || !camera.Project({5.0F, 2.5F, 10.0F}, clip) || std::fabs(clip.x - 0.5F) > 0.0001F || std::fabs(clip.y - 0.5F) > 0.0001F || camera.Project({50.0F, 0.0F, 10.0F}, clip) || camera.LastError() != RenderCameraError::OutsideClip) return 1;
    if (!camera.Initialize({RenderCameraMode::Perspective, {}, 5.0F, 90.0F, 1.0F, 0.1F, 100.0F}) || !camera.Project({5.0F, 0.0F, 5.0F}, clip) || std::fabs(clip.x - 1.0F) > 0.0001F || camera.Project({0.0F, 0.0F, 0.05F}, clip) || camera.LastError() != RenderCameraError::BehindCamera) return 1;
    if (!camera.Initialize({RenderCameraMode::Perspective, {}, 5.0F, 90.0F, 1.0F, 0.1F, 100.0F, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}}) || !camera.Project({5.0F, 0.0F, 0.0F}, clip) || std::fabs(clip.x) > 0.0001F || std::fabs(clip.y) > 0.0001F || !camera.Project({5.0F, 5.0F, 0.0F}, clip) || std::fabs(clip.y - 1.0F) > 0.0001F) return 1;
    if (camera.Initialize({RenderCameraMode::Perspective, {}, 5.0F, 60.0F, 1.0F, 0.1F, 10.0F, {}, {0.0F, 1.0F, 0.0F}}) || camera.LastError() != RenderCameraError::InvalidOrientation || camera.Initialize({RenderCameraMode::Orthographic, {}, 5.0F, 60.0F, 1.0F, 0.1F, 10.0F, {0.0F, 1.0F, 0.0F}, {0.0F, 2.0F, 0.0F}}) || camera.LastError() != RenderCameraError::InvalidOrientation) return 1;
    std::printf("RENDER_CAMERA_SMOKE_OK orthographic=1 perspective=1 orientation=1 clip=1 invalid=1\n"); return 0;
}
