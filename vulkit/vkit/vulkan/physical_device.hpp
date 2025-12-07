#pragma once

#ifndef VKIT_ENABLE_PHYSICAL_DEVICE
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_PHYSICAL_DEVICE"
#endif

#include "tkit/container/static_array.hpp"
#include "vkit/vulkan/instance.hpp"

namespace VKit
{
enum QueueType : u32
{
    Queue_Graphics = 0,
    Queue_Compute = 1,
    Queue_Transfer = 2,
    Queue_Present = 3,
};
const char *ToString(QueueType p_Type);
/**
 * @brief Represents a Vulkan physical device and its features.
 *
 * Encapsulates the Vulkan physical device handle and provides access to its
 * features, properties, and queue support. Includes methods to query and
 * manage device-specific details.
 *
 * If the selected Vulkan API version does not support certain features (e.g.,
 * 1.1/1.2/1.3), the related properties and features will be ignored.
 */
class VKIT_API PhysicalDevice
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
        Features();

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
#ifdef VKIT_API_VERSION_1_4
        VkPhysicalDeviceVulkan14Properties Vulkan14{};
#endif
    };

#ifdef VK_KHR_surface
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR Capabilities;
        TKit::StaticArray128<VkSurfaceFormatKHR> Formats;
        TKit::StaticArray8<VkPresentModeKHR> PresentModes;
    };
#endif

    /**
     * @brief A helper class for selecting a Vulkan physical device.
     *
     * Allows you to define requirements such as supported extensions, memory
     * capacity, queue capabilities, and device type. Evaluates available devices
     * and selects the one that best matches the criteria.
     */
    class Selector
    {
      public:
        using Flags = u16;
        /**
         * @brief Flags for specifying criteria when selecting a physical device.
         *
         * Used to filter devices based on required queue types, memory capabilities,
         * and extension support.
         */
        enum FlagBits : Flags
        {
            Flag_None = 0,
            Flag_AnyType = 1 << 0,
            Flag_RequireDedicatedComputeQueue = 1 << 1,
            Flag_RequireDedicatedTransferQueue = 1 << 2,
            Flag_RequireSeparateComputeQueue = 1 << 3,
            Flag_RequireSeparateTransferQueue = 1 << 4,
            Flag_PortabilitySubset = 1 << 5,
            Flag_RequireGraphicsQueue = 1 << 6,
            Flag_RequireComputeQueue = 1 << 7,
            Flag_RequireTransferQueue = 1 << 8,
            Flag_RequirePresentQueue = 1 << 9
        };

        Selector(const Instance *p_Instance) : m_Instance(p_Instance)
        {
        }

        /**
         * @brief Selects the best matching physical device.
         *
         * Based on the specified requirements and preferences, this method selects a
         * Vulkan physical device and returns it. If no suitable device is found, an error is returned.
         *
         * @return A `Result` containing the selected PhysicalDevice or an error.
         */
        FormattedResult<PhysicalDevice> Select();

        /**
         * @brief Lists all available physical devices along with their evaluation results.
         *
         * Enumerates all Vulkan physical devices and evaluates them based on the selector's
         * criteria. Provides detailed results for each device.
         *
         * @return A `Result` containing an array of formatted results for each physical device.
         */
        Result<TKit::StaticArray4<FormattedResult<PhysicalDevice>>> Enumerate();

        Selector &SetName(const char *p_Name);
        Selector &PreferType(Type p_Type);

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

        Selector &RequireFeatures(const Features &p_Features);

        Selector &SetFlags(Flags p_Flags);
        Selector &AddFlags(Flags p_Flags);
        Selector &RemoveFlags(Flags p_Flags);

#ifdef VK_KHR_surface
        Selector &SetSurface(VkSurfaceKHR p_Surface);
#endif

      private:
        FormattedResult<PhysicalDevice> judgeDevice(VkPhysicalDevice p_Device) const;

        const Instance *m_Instance;
        const char *m_Name = nullptr;

        u32 m_RequiredApiVersion = VKIT_MAKE_VERSION(0, 1, 0, 0);
        u32 m_RequestedApiVersion = VKIT_MAKE_VERSION(0, 1, 0, 0);

#ifdef VK_KHR_surface
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
#endif
        Type m_PreferredType = Discrete;

        Flags m_Flags = 0;

        VkDeviceSize m_RequiredMemory = 0;
        VkDeviceSize m_RequestedMemory = 0;

        TKit::StaticArray256<std::string> m_RequiredExtensions;
        TKit::StaticArray256<std::string> m_RequestedExtensions;

        Features m_RequiredFeatures{};
    };

    using Flags = u16;
    /**
     * @brief Flags to describe physical device capabilities.
     *
     * These flags indicate features like dedicated or separate queues, graphics
     * and compute support, and portability subset availability.
     */
    enum FlagBits : Flags
    {
        Flag_None = 0,
        Flag_Optimal = 1 << 0,
        Flag_HasDedicatedComputeQueue = 1 << 1,
        Flag_HasDedicatedTransferQueue = 1 << 2,
        Flag_HasSeparateTransferQueue = 1 << 3,
        Flag_HasSeparateComputeQueue = 1 << 4,
        Flag_PortabilitySubset = 1 << 5,
        Flag_HasGraphicsQueue = 1 << 6,
        Flag_HasComputeQueue = 1 << 7,
        Flag_HasTransferQueue = 1 << 8,
        Flag_HasPresentQueue = 1 << 9
    };

    struct Info
    {
        Type Type;
        Flags Flags;

        u32 ApiVersion;

        TKit::Array4<u32> FamilyIndices;
        TKit::StaticArray8<VkQueueFamilyProperties> QueueFamilies;

        // std string because extension names are "locally" allocated
        TKit::StaticArray256<std::string> EnabledExtensions;
        TKit::StaticArray256<std::string> AvailableExtensions;

        Features EnabledFeatures{};
        Features AvailableFeatures{};

        Properties Properties{};
    };

    PhysicalDevice() = default;
    PhysicalDevice(VkPhysicalDevice p_Device, const Info &p_Info) : m_Device(p_Device), m_Info(p_Info)
    {
    }

    bool AreFeaturesSupported(const Features &p_Features) const;
    bool AreFeaturesEnabled(const Features &p_Features) const;

    bool EnableFeatures(const Features &p_Features);

    /**
     * @brief Add a non-core feature that requires an extension to be supported and that is potentially not present
     * in the user's current header version.
     *
     * The availability of such features will not be checked. It is up to you to provide the required support
     * through extensions.
     *
     * @param p_Feature The feature, which must remain in scope until a logical device has been created.
     */
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
    Result<SwapChainSupportDetails> QuerySwapChainSupport(const Instance::Proxy &p_Instance,
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
