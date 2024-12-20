#pragma once

#include "vkit/backend/instance.hpp"

namespace VKit
{
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
        TKit::StaticArray16<VkSurfaceFormatKHR> Formats;
        TKit::StaticArray8<VkPresentModeKHR> PresentModes;
    };

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
        /**
         * @brief Flags for specifying criteria when selecting a physical device.
         *
         * Used to filter devices based on required queue types, memory capabilities,
         * and extension support.
         */
        enum FlagBits : u16
        {
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
        using Flags = u16;

        explicit Selector(const Instance *p_Instance) noexcept;

        /**
         * @brief Selects the best matching physical device.
         *
         * Based on the specified requirements and preferences, this method selects a
         * Vulkan physical device and returns it. If no suitable device is found, an error is returned.
         *
         * @return A result containing the selected PhysicalDevice or an error.
         */
        FormattedResult<PhysicalDevice> Select() const noexcept;

        /**
         * @brief Lists all available physical devices along with their evaluation results.
         *
         * Enumerates all Vulkan physical devices and evaluates them based on the selector's
         * criteria. Provides detailed results for each device.
         *
         * @return A result containing an array of formatted results for each physical device.
         */
        Result<TKit::StaticArray4<FormattedResult<PhysicalDevice>>> Enumerate() const noexcept;

        Selector &SetName(const char *p_Name) noexcept;
        Selector &PreferType(Type p_Type) noexcept;

        Selector &RequireExtension(const char *p_Extension) noexcept;
        Selector &RequireExtensions(std::span<const char *const> p_Extensions) noexcept;

        Selector &RequestExtension(const char *p_Extension) noexcept;
        Selector &RequestExtensions(std::span<const char *const> p_Extensions) noexcept;

        Selector &RequireMemory(const VkDeviceSize p_Size) noexcept;
        Selector &RequestMemory(const VkDeviceSize p_Size) noexcept;

        Selector &RequireFeatures(const Features &p_Features) noexcept;

        Selector &SetFlags(Flags p_Flags) noexcept;
        Selector &AddFlags(Flags p_Flags) noexcept;
        Selector &RemoveFlags(Flags p_Flags) noexcept;

        Selector &SetSurface(VkSurfaceKHR p_Surface) noexcept;

      private:
        FormattedResult<PhysicalDevice> judgeDevice(VkPhysicalDevice p_Device) const noexcept;

        const Instance *m_Instance;
        const char *m_Name = nullptr;

        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
        Type m_PreferredType = Discrete;

        Flags m_Flags = 0;

        VkDeviceSize m_RequiredMemory = 0;
        VkDeviceSize m_RequestedMemory = 0;

        TKit::StaticArray128<std::string> m_RequiredExtensions;
        TKit::StaticArray128<std::string> m_RequestedExtensions;

        Features m_RequiredFeatures{};
    };

    /**
     * @brief Flags to describe physical device capabilities.
     *
     * These flags indicate features like dedicated or separate queues, graphics
     * and compute support, and portability subset availability.
     */
    enum FlagBits : u16
    {
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
    using Flags = u16;

    struct Info
    {
        Type Type;
        Flags Flags;

        u32 GraphicsIndex;
        u32 ComputeIndex;
        u32 TransferIndex;
        u32 PresentIndex;
        TKit::StaticArray8<VkQueueFamilyProperties> QueueFamilies;

        // std string because extension names are "locally" allocated
        TKit::StaticArray128<std::string> EnabledExtensions;
        TKit::StaticArray128<std::string> AvailableExtensions;

        Features EnabledFeatures{};
        Features AvailableFeatures{};

        Properties Properties{};
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
