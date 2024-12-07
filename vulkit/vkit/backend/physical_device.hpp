#pragma once

#include "vkit/backend/instance.hpp"

namespace VKit
{
enum PhysicalDeviceFlags : u16
{
    PhysicalDeviceFlags_Optimal = 1 << 0,
    PhysicalDeviceFlags_HasDedicatedComputeQueue = 1 << 1,
    PhysicalDeviceFlags_HasDedicatedTransferQueue = 1 << 2,
    PhysicalDeviceFlags_HasSeparateTransferQueue = 1 << 3,
    PhysicalDeviceFlags_HasSeparateComputeQueue = 1 << 4,
    PhysicalDeviceFlags_PortabilitySubset = 1 << 5,
    PhysicalDeviceFlags_HasGraphicsQueue = 1 << 6,
    PhysicalDeviceFlags_HasComputeQueue = 1 << 7,
    PhysicalDeviceFlags_HasTransferQueue = 1 << 8,
    PhysicalDeviceFlags_HasPresentQueue = 1 << 9
};

enum PhysicalDeviceSelectorFlags : u16
{
    PhysicalDeviceSelectorFlags_AnyType = 1 << 0,
    PhysicalDeviceSelectorFlags_RequireDedicatedComputeQueue = 1 << 1,
    PhysicalDeviceSelectorFlags_RequireDedicatedTransferQueue = 1 << 2,
    PhysicalDeviceSelectorFlags_RequireSeparateComputeQueue = 1 << 3,
    PhysicalDeviceSelectorFlags_RequireSeparateTransferQueue = 1 << 4,
    PhysicalDeviceSelectorFlags_PortabilitySubset = 1 << 5,
    PhysicalDeviceSelectorFlags_RequireGraphicsQueue = 1 << 6,
    PhysicalDeviceSelectorFlags_RequireComputeQueue = 1 << 7,
    PhysicalDeviceSelectorFlags_RequireTransferQueue = 1 << 8,
    PhysicalDeviceSelectorFlags_RequirePresentQueue = 1 << 9
};

// If instance API version do not support 1.1/1.2/1.3 features event though the version macros are defined,
// the 1.1/1.2/1.3 structs will be ignored

class PhysicalDevice
{
  public:
    enum Type
    {
        Discrete = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
        Integrated = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
        Virtual = VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
        CPU = VK_PHYSICAL_DEVICE_TYPE_CPU,
        Other = VK_PHYSICAL_DEVICE_TYPE_OTHER
    };

    struct Features
    {
        VkPhysicalDeviceFeatures Core{};
#ifdef VKIT_API_VERSION_1_2
        VkPhysicalDeviceVulkan11Features Vulkan11{};
        VkPhysicalDeviceVulkan12Features Vulkan12{};
#endif
#ifdef VKIT_API_VERSION_1_3
        VkPhysicalDeviceVulkan13Features Vulkan13{};
#endif
    };

    struct Properties
    {
        VkPhysicalDeviceProperties Core{};
        VkPhysicalDeviceMemoryProperties Memory{};
#ifdef VKIT_API_VERSION_1_2
        VkPhysicalDeviceVulkan11Properties Vulkan11{};
        VkPhysicalDeviceVulkan12Properties Vulkan12{};
#endif
#ifdef VKIT_API_VERSION_1_3
        VkPhysicalDeviceVulkan13Properties Vulkan13{};
#endif
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR Capabilities;
        DynamicArray<VkSurfaceFormatKHR> Formats;
        DynamicArray<VkPresentModeKHR> PresentModes;
    };

    struct Info
    {
        Type Type;
        u16 Flags;

        u32 GraphicsIndex;
        u32 ComputeIndex;
        u32 TransferIndex;
        u32 PresentIndex;
        DynamicArray<VkQueueFamilyProperties> QueueFamilies;

        // std string because extension names are "locally" allocated
        DynamicArray<std::string> EnabledExtensions;
        DynamicArray<std::string> AvailableExtensions;

        Features EnabledFeatures{};
        Features AvailableFeatures{};

        Properties Properties{};
    };

    class Selector
    {
      public:
        explicit Selector(const Instance *p_Instance) noexcept;

        FormattedResult<PhysicalDevice> Select() const noexcept;
        Result<DynamicArray<FormattedResult<PhysicalDevice>>> Enumerate() const noexcept;

        Selector &SetName(const char *p_Name) noexcept;
        Selector &PreferType(Type p_Type) noexcept;

        Selector &RequireExtension(const char *p_Extension) noexcept;
        Selector &RequireExtensions(std::span<const char *const> p_Extensions) noexcept;

        Selector &RequestExtension(const char *p_Extension) noexcept;
        Selector &RequestExtensions(std::span<const char *const> p_Extensions) noexcept;

        Selector &RequireMemory(const VkDeviceSize p_Size) noexcept;
        Selector &RequestMemory(const VkDeviceSize p_Size) noexcept;

        Selector &RequireFeatures(const Features &p_Features) noexcept;

        Selector &SetFlags(u16 p_Flags) noexcept;
        Selector &AddFlags(u16 p_Flags) noexcept;
        Selector &RemoveFlags(u16 p_Flags) noexcept;

        Selector &SetSurface(VkSurfaceKHR p_Surface) noexcept;

      private:
        FormattedResult<PhysicalDevice> judgeDevice(VkPhysicalDevice p_Device) const noexcept;

        const Instance *m_Instance;
        const char *m_Name = nullptr;

        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
        Type m_PreferredType = Discrete;

        u16 m_Flags = 0;

        VkDeviceSize m_RequiredMemory = 0;
        VkDeviceSize m_RequestedMemory = 0;

        DynamicArray<std::string> m_RequiredExtensions;
        DynamicArray<std::string> m_RequestedExtensions;

        Features m_RequiredFeatures{};
    };

    PhysicalDevice() noexcept = default;
    PhysicalDevice(VkPhysicalDevice p_Device, const Info &p_Info) noexcept;

    bool IsExtensionSupported(const char *p_Extension) const noexcept;
    bool IsExtensionEnabled(const char *p_Extension) const noexcept;

    bool EnableExtension(const char *p_Extension) noexcept;

    VkPhysicalDevice GetDevice() const noexcept;
    const Info &GetInfo() const noexcept;

    Result<SwapChainSupportDetails> QuerySwapChainSupport(const Instance &p_Instance,
                                                          VkSurfaceKHR p_Surface) const noexcept;

    explicit(false) operator VkPhysicalDevice() const noexcept;
    explicit(false) operator bool() const noexcept;

  private:
    VkPhysicalDevice m_Device = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit
