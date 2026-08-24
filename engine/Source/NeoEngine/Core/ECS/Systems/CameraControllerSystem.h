#pragma once

#include "../Components/Transform.h"
#include "../Components/CameraComponent.h"
#include <GLFW/glfw3.h>

namespace NeoEngine
{

class CameraControllerSystem
{
public:
    float speed       = 2.5f;
    float sensitivity = 0.002f;
    double lastX = 400, lastY = 300;
    bool firstMouse = true;

    void Update(EntityManager& em, GLFWwindow* window, float dt)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        if (firstMouse) { lastX=xpos; lastY=ypos; firstMouse=false; }

        float dx = static_cast<float>(xpos - lastX);
        float dy = static_cast<float>(lastY - ypos);
        lastX = xpos; lastY = ypos;

        auto entities = em.GetEntitiesWith<Transform, CameraComponent>();
        for (auto e : entities)
        {
            auto& t = em.GetComponent<Transform>(e);

            Vec3 forward = t.rotation.RotateVector({0,0,-1});
            Vec3 right   = t.rotation.RotateVector({1,0, 0});
            Vec3 up      = {0,1,0};

            forward.Normalize();
            right.Normalize();

            if (glfwGetKey(window, GLFW_KEY_W)==GLFW_PRESS) t.position += forward * (speed*dt);
            if (glfwGetKey(window, GLFW_KEY_S)==GLFW_PRESS) t.position -= forward * (speed*dt);
            if (glfwGetKey(window, GLFW_KEY_A)==GLFW_PRESS) t.position -= right   * (speed*dt);
            if (glfwGetKey(window, GLFW_KEY_D)==GLFW_PRESS) t.position += right   * (speed*dt);
            if (glfwGetKey(window, GLFW_KEY_Q)==GLFW_PRESS) t.position -= up      * (speed*dt);
            if (glfwGetKey(window, GLFW_KEY_E)==GLFW_PRESS) t.position += up      * (speed*dt);

            Quat qYaw   = Quat::FromAxisAngle({0,1,0}, dx*sensitivity);
            Quat qPitch = Quat::FromAxisAngle({1,0,0}, dy*sensitivity);
            t.rotation  = (qYaw * t.rotation * qPitch);
            t.rotation.Normalize();
        }
    }
};

} // namespace NeoEngine
