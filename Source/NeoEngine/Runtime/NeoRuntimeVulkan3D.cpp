#include "NeoRuntime.h"

#include <algorithm>

namespace NeoEngine {

bool NeoRuntime::RenderScene3D() {
    if (m_State != RuntimeState::Initialized || !m_Scene ||
        !m_EnableVulkan3DRenderer) {
        m_LastError = RuntimeError::InvalidState;
        return false;
    }

    const uint16_t width = std::max<uint16_t>(1U, m_RenderWidth);
    const uint16_t height = std::max<uint16_t>(1U, m_RenderHeight);

    if (!m_SceneMeshes)
        m_SceneMeshes = std::make_unique<SceneMeshAdapter>();

    if (!m_SceneRenderAdapter)
        m_SceneRenderAdapter = std::make_unique<SceneRenderAdapter>();

    if (!m_SceneCamera) {
        m_SceneCamera = std::make_unique<RenderCamera>();

        RenderCameraConfig camera = m_SceneCameraConfig;

        if (camera.aspect <= 0.0F) {
            camera.aspect =
                static_cast<float>(width) /
                static_cast<float>(height);
        }

        if (!m_SceneCamera->Initialize(camera)) {
            m_SceneCamera.reset();
            m_LastError = RuntimeError::Vulkan3DRenderFailed;
            return false;
        }
    }

    if (!m_VulkanRenderer) {
        m_VulkanRenderer = std::make_unique<Vulkan3DRenderer>();

        if (!m_VulkanRenderer->Initialize(
                width,
                height,
                "FauzanEngine 3D")) {
            m_VulkanRenderer.reset();
            m_LastError = RuntimeError::Vulkan3DRenderFailed;
            return false;
        }
    }

    if (!m_SceneRenderAdapter->DrawVulkan3D(
            *m_Scene,
            *m_SceneMeshes,
            *m_SceneCamera,
            *m_VulkanRenderer)) {
        m_LastError = RuntimeError::Vulkan3DRenderFailed;
        return false;
    }

    m_LastError = RuntimeError::None;
    return true;
}

} // namespace NeoEngine
