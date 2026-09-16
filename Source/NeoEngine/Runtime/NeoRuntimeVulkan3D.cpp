#include "NeoRuntime.h"

#include <algorithm>

namespace NeoEngine {

bool NeoRuntime::RenderScene3D() {
    if (m_State != RuntimeState::Initialized || !m_Scene) {
        m_LastError = RuntimeError::InvalidState;
        return false;
    }

    if (!m_SceneMeshes) m_SceneMeshes = std::make_unique<SceneMeshAdapter>();
    if (!m_SceneCamera) {
        m_SceneCamera = std::make_unique<RenderCamera>();
        RenderCameraConfig camera{};
        camera.mode = RenderCameraMode::Perspective;
        camera.position = {0.0F, 2.0F, -6.0F};
        camera.verticalFovDegrees = 60.0F;
        camera.aspect = 16.0F / 9.0F;
        camera.nearPlane = 0.1F;
        camera.farPlane = 2000.0F;
        camera.forward = {0.0F, 0.0F, 1.0F};
        camera.up = {0.0F, 1.0F, 0.0F};
        if (!m_SceneCamera->Initialize(camera)) {
            m_LastError = RuntimeError::Vulkan3DRenderFailed;
            return false;
        }
    }
    if (!m_SceneRenderAdapter) m_SceneRenderAdapter = std::make_unique<SceneRenderAdapter>();
    if (!m_VulkanRenderer) {
        m_VulkanRenderer = std::make_unique<Vulkan3DRenderer>();
        if (!m_VulkanRenderer->Initialize(1280U, 720U, "FauzanEngine 3D")) {
            m_VulkanRenderer.reset();
            m_LastError = RuntimeError::Vulkan3DRenderFailed;
            return false;
        }
    }

    if (!m_SceneRenderAdapter->DrawVulkan3D(*m_Scene, *m_SceneMeshes, *m_SceneCamera, *m_VulkanRenderer)) {
        m_LastError = RuntimeError::Vulkan3DRenderFailed;
        return false;
    }
    m_LastError = RuntimeError::None;
    return true;
}

} // namespace NeoEngine
