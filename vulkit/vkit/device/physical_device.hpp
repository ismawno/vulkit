#pragma once

#ifndef VKIT_ENABLE_PHYSICAL_DEVICE
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_PHYSICAL_DEVICE"
#endif

#include "tkit/container/array.hpp"
#include "vkit/vulkan/instance.hpp"

namespace VKit
{

enum DeviceType : u32
{
    Device_Discrete = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
    Device_Integrated = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
    Device_Virtual = VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
    Device_CPU = VK_PHYSICAL_DEVICE_TYPE_CPU,
    Device_Other = VK_PHYSICAL_DEVICE_TYPE_OTHER
};

using DeviceSelectorFlags = u16;
enum DeviceSelectorFlagBits : DeviceSelectorFlags
{
    DeviceSelectorFlag_AnyType = 1 << 0,
    DeviceSelectorFlag_RequireDedicatedComputeQueue = 1 << 1,
    DeviceSelectorFlag_RequireDedicatedTransferQueue = 1 << 2,
    DeviceSelectorFlag_RequireSeparateComputeQueue = 1 << 3,
    DeviceSelectorFlag_RequireSeparateTransferQueue = 1 << 4,
    DeviceSelectorFlag_PortabilitySubset = 1 << 5,
    DeviceSelectorFlag_RequireGraphicsQueue = 1 << 6,
    DeviceSelectorFlag_RequireComputeQueue = 1 << 7,
    DeviceSelectorFlag_RequireTransferQueue = 1 << 8,
    DeviceSelectorFlag_RequirePresentQueue = 1 << 9
};

using DeviceFlags = u16;
enum DeviceFlagBits : DeviceFlags
{
    DeviceFlag_Optimal = 1 << 0,
    DeviceFlag_HasDedicatedComputeQueue = 1 << 1,
    DeviceFlag_HasDedicatedTransferQueue = 1 << 2,
    DeviceFlag_HasSeparateTransferQueue = 1 << 3,
    DeviceFlag_HasSeparateComputeQueue = 1 << 4,
    DeviceFlag_PortabilitySubset = 1 << 5,
    DeviceFlag_HasGraphicsQueue = 1 << 6,
    DeviceFlag_HasComputeQueue = 1 << 7,
    DeviceFlag_HasTransferQueue = 1 << 8,
    DeviceFlag_HasPresentQueue = 1 << 9
};

struct DeviceFeatures
{
#if defined(VKIT_API_VERSION_1_1) || defined(VK_KHR_get_physical_device_properties2)
    VkPhysicalDeviceFeatures2KHR CreateChain(u32 p_ApiVersion);
#endif

    VkPhysicalDeviceFeatures Core{};
#ifdef VKIT_API_VERSION_1_2
    VkPhysicalDeviceVulkan11Features Vulkan11{};
    VkPhysicalDeviceVulkan12Features Vulkan12{};
#endif
#ifdef VKIT_API_VERSION_1_3
    VkPhysicalDeviceVulkan13Features Vulkan13{};
#endif
#ifdef VKIT_API_VERSION_1_4
    VkPhysicalDeviceVulkan14Features Vulkan14{};
#endif
    void *Next;
};

struct DeviceProperties
{
#if defined(VKIT_API_VERSION_1_1) || defined(VK_KHR_get_physical_device_properties2)
    VkPhysicalDeviceProperties2 CreateChain(u32 p_ApiVersion);
#endif

    VkPhysicalDeviceProperties Core{};
    VkPhysicalDeviceMemoryProperties Memory{};
#ifdef VKIT_API_VERSION_1_2
    VkPhysicalDeviceVulkan11Properties Vulkan11{};
    VkPhysicalDeviceVulkan12Properties Vulkan12{};
#endif
#ifdef VKIT_API_VERSION_1_3
    VkPhysicalDeviceVulkan13Properties Vulkan13{};
#endif
#ifdef VKIT_API_VERSION_1_4
    VkPhysicalDeviceVulkan14Properties Vulkan14{};
#endif
    void *Next;
};

class PhysicalDevice
{
  public:
#ifdef VK_KHR_surface
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR Capabilities;
        TKit::Array128<VkSurfaceFormatKHR> Formats;
        TKit::Array8<VkPresentModeKHR> PresentModes;
    };
#endif

    class Selector
    {
      public:
        Selector(const Instance *p_Instance);

        VKIT_NO_DISCARD Result<PhysicalDevice> Select() const;

        VKIT_NO_DISCARD Result<TKit::Array4<Result<PhysicalDevice>>> Enumerate() const;

        Selector &SetName(const char *p_Name);
        Selector &PreferType(DeviceType p_Type);

        Selector &RequireApiVersion(u32 p_Version);
        Selector &RequireApiVersion(u32 p_Major, u32 p_Minor, u32 p_Patch);

        Selector &RequestApiVersion(u32 p_Version);
        Selector &RequestApiVersion(u32 p_Major, u32 p_Minor, u32 p_Patch);

        Selector &RequireExtension(const char *p_Extension);
        Selector &RequireExtensions(TKit::Span<const char *const> p_Extensions);

        Selector &RequestExtension(const char *p_Extension);
        Selector &RequestExtensions(TKit::Span<const char *const> p_Extensions);

        Selector &RequireMemory(const VkDeviceSize p_Size);
        Selector &RequestMemory(const VkDeviceSize p_Size);

        Selector &RequireFeatures(const DeviceFeatures &p_Features);

        Selector &SetFlags(DeviceSelectorFlags p_Flags);
        Selector &AddFlags(DeviceSelectorFlags p_Flags);
        Selector &RemoveFlags(DeviceSelectorFlags p_Flags);

#ifdef VK_KHR_surface
        Selector &SetSurface(VkSurfaceKHR p_Surface);
#endif

      private:
        VKIT_NO_DISCARD Result<PhysicalDevice> judgeDevice(VkPhysicalDevice p_Device) const;

        const Instance *m_Instance;
        const char *m_Name = nullptr;

        u32 m_RequiredApiVersion = VKIT_MAKE_VERSION(0, 1, 0, 0);
        u32 m_RequestedApiVersion = VKIT_MAKE_VERSION(0, 1, 0, 0);

#ifdef VK_KHR_surface
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
#endif
        DeviceType m_PreferredType = Device_Discrete;
        DeviceSelectorFlags m_Flags = 0;

        VkDeviceSize m_RequiredMemory = 0;
        VkDeviceSize m_RequestedMemory = 0;

        TKit::Array256<std::string> m_RequiredExtensions;
        TKit::Array256<std::string> m_RequestedExtensions;

        DeviceFeatures m_RequiredFeatures{};
    };

    struct Info
    {
        DeviceType Type;

        u32 ApiVersion;

        TKit::FixedArray4<u32> FamilyIndices;
        TKit::Array8<VkQueueFamilyProperties> QueueFamilies;

        // std string because extension names are "locally" allocated
        TKit::Array256<std::string> EnabledExtensions;
        TKit::Array256<std::string> AvailableExtensions;

        DeviceFeatures EnabledFeatures{};
        DeviceFeatures AvailableFeatures{};

        DeviceProperties Properties{};
        DeviceFlags Flags;
    };

    PhysicalDevice() = default;
    PhysicalDevice(VkPhysicalDevice p_Device, const Info &p_Info) : m_Device(p_Device), m_Info(p_Info)
    {
    }

    bool AreFeaturesSupported(const DeviceFeatures &p_Features) const;
    bool AreFeaturesEnabled(const DeviceFeatures &p_Features) const;

    bool EnableFeatures(const DeviceFeatures &p_Features);

    template <typename T> void EnableExtensionBoundFeature(T *p_Feature)
    {
        void *next = m_Info.EnabledFeatures.Next;
        p_Feature->pNext = next;
        m_Info.EnabledFeatures.Next = p_Feature;
    }

    bool IsExtensionSupported(const char *p_Extension) const;
    bool IsExtensionEnabled(const char *p_Extension) const;

    bool EnableExtension(const char *p_Extension);

    VkPhysicalDevice GetHandle() const
    {
        return m_Device;
    }
    const Info &GetInfo() const
    {
        return m_Info;
    }

#ifdef VK_KHR_surface
    VKIT_NO_DISCARD Result<SwapChainSupportDetails> QuerySwapChainSupport(const Instance::Proxy &p_Instance,
                                                                        VkSurfaceKHR p_Surface) const;
#endif

    operator VkPhysicalDevice() const
    {
        return m_Device;
    }
    operator bool() const
    {
        return m_Device != VK_NULL_HANDLE;
    }

  private:
    VkPhysicalDevice m_Device = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit
