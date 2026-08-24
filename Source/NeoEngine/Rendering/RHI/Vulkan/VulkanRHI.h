#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <functional>
namespace NeoEngine {
class VulkanRHI {
    VkInstance m_Instance=VK_NULL_HANDLE; VkPhysicalDevice m_GPU=VK_NULL_HANDLE;
    VkDevice m_Device=VK_NULL_HANDLE; VkQueue m_GraphicsQueue=VK_NULL_HANDLE;
    VkSwapchainKHR m_Swapchain=VK_NULL_HANDLE; std::vector<VkImage> m_SwapchainImages;
    VkFormat m_SwapchainFormat; int m_Width=0,m_Height=0; bool m_Initialized=false;
public:
    static VulkanRHI& Get();
    bool Init(void* nativeWindow,int w,int h,const char* appName="FauzanEngine");
    void Shutdown(); void BeginFrame(); void EndFrame(); void Present();
    VkDevice GetDevice()const{return m_Device;}
    VkPhysicalDevice GetGPU()const{return m_GPU;}
    int GetWidth()const{return m_Width;} int GetHeight()const{return m_Height;}
    bool IsInitialized()const{return m_Initialized;}
};
}
