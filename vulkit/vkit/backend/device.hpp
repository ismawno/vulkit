#pragma once

#include "vkit/core/instance.hpp"
#include "vkit/core/alias.hpp"
#include <vulkan/vulkan.hpp>
#include "tkit/profiling/vulkan.hpp"

namespace VKit
{
class VKIT_API Device : public TKit::RefCounted<Device>
{
    TKIT_NON_COPYABLE(Device)
  public:
    struct Specs
    {
        const char *TargetDevice = nullptr;
        VkSurfaceKHR Surface = VK_NULL_HANDLE;
        std::span<const char *> RequiredExtensions;
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR Capabilities;
        DynamicArray<VkSurfaceFormatKHR> Formats;
        DynamicArray<VkPresentModeKHR> PresentModes;
    };

    struct QueueFamilyIndices
    {
        u32 GraphicsFamily;
        u32 PresentFamily;
        bool GraphicsFamilyHasValue = false;
        bool PresentFamilyHasValue = false;
    };

    explicit Device(VkSurfaceKHR p_Surface) noexcept;
    ~Device() noexcept;

    void WaitIdle() noexcept;

    VkFormat FindSupportedFormat(std::span<const VkFormat> p_Candidates, VkImageTiling p_Tiling,
                                 VkFormatFeatureFlags p_Features) const noexcept;

    SwapChainSupportDetails QuerySwapChainSupport(VkSurfaceKHR p_Surface) const noexcept;
    QueueFamilyIndices FindQueueFamilies(VkSurfaceKHR p_Surface) const noexcept;

    VkMemoryPropertyFlags FindMemoryType(u32 p_TypeFilter, VkMemoryPropertyFlags p_Properties) const noexcept;

    bool IsSuitable(VkSurfaceKHR p_Surface) const noexcept;

    VkDevice GetDevice() const noexcept;
    VkPhysicalDevice GetPhysicalDevice() const noexcept;

    VkQueue GetGraphicsQueue() const noexcept;
    VkQueue GetPresentQueue() const noexcept;

    const VkPhysicalDeviceProperties &GetProperties() const noexcept;

    std::mutex &GraphicsMutex() noexcept;
    std::mutex &PresentMutex() noexcept;

    void LockQueues() noexcept;
    void UnlockQueues() noexcept;

    VkCommandBuffer BeginSingleTimeCommands(VkCommandPool p_Pool = VK_NULL_HANDLE) const noexcept;
    void EndSingleTimeCommands(VkCommandBuffer p_CommandBuffer, VkCommandPool p_Pool = VK_NULL_HANDLE) const noexcept;

  private:
    void pickPhysicalDevice(VkSurfaceKHR p_Surface) noexcept;
    void createLogicalDevice(VkSurfaceKHR p_Surface) noexcept;
    void createCommandPool(VkSurfaceKHR p_Surface) noexcept;

    TKit::Ref<Instance> m_Instance;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties m_Properties;

    VkDevice m_Device;
    VkCommandPool m_CommandPool;

    VkQueue m_GraphicsQueue;
    VkQueue m_PresentQueue;

    std::mutex m_GraphicsMutex;
    std::mutex m_PresentMutex;

#ifdef TKIT_ENABLE_VULKAN_PROFILING
    TKit::VkProfilingContext m_ProfilingContext;
    VkCommandBuffer m_ProfilingCommandBuffer;

  public:
    TKit::VkProfilingContext GetProfilingContext() const noexcept;
#endif
};
} // namespace VKit