#pragma once

#ifndef VKIT_ENABLE_LOGICAL_DEVICE
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_LOGICAL_DEVICE"
#endif

#include "vkit/vulkan/physical_device.hpp"

#ifndef VKIT_MAX_QUEUES_PER_FAMILY
#    define VKIT_MAX_QUEUES_PER_FAMILY 4
#endif

#if VKIT_MAX_QUEUES_PER_FAMILY < 1
#    error "[VULKIT] Maximum queues per family must be greater than 0"
#endif

namespace VKit
{
enum class VKIT_API QueueType : u32
{
    Graphics = 0,
    Compute = 1,
    Transfer = 2,
    Present = 3
};

/**
 * @brief Represents a Vulkan logical device and its associated state.
 *
 * The logical device manages queues, resources, and interactions with a physical device.
 * It provides methods for resource allocation and command submission to the Vulkan API.
 */
class VKIT_API LogicalDevice
{
  public:
    struct Proxy
    {
        VkDevice Device = VK_NULL_HANDLE;
        const VkAllocationCallbacks *AllocationCallbacks = nullptr;
        const Vulkan::DeviceTable *Table = nullptr;

        operator VkDevice() const
        {
            return Device;
        }
        operator bool() const
        {
            return Device != VK_NULL_HANDLE;
        }
    };

    /**
     * @brief Defines the priorities for device queues.
     *
     * The amount of queues will be determined by the number of priorities provided.
     */
    struct QueuePriorities
    {
        TKit::StaticArray16<f32> RequiredPriorities;
        TKit::StaticArray16<f32> RequestedPriorities;
    };

    class Builder
    {
      public:
        Builder(const Instance *p_Instance, const PhysicalDevice *p_PhysicalDevice)
            : m_Instance(p_Instance), m_PhysicalDevice(p_PhysicalDevice),
              m_Priorities(p_PhysicalDevice->GetInfo().QueueFamilies.GetSize())
        {
        }

        Builder &RequireQueue(QueueType p_Type, u32 p_Count = 1, f32 p_Priority = 1.f);
        Builder &RequestQueue(QueueType p_Type, u32 p_Count = 1, f32 p_Priority = 1.f);

        Builder &RequireQueue(u32 p_Family, u32 p_Count = 1, f32 p_Priority = 1.f);
        Builder &RequestQueue(u32 p_Family, u32 p_Count = 1, f32 p_Priority = 1.f);

        FormattedResult<LogicalDevice> Build();

      private:
        const Instance *m_Instance;
        const PhysicalDevice *m_PhysicalDevice;

        TKit::StaticArray8<QueuePriorities> m_Priorities;
    };

    struct Info
    {
        Instance Instance;
        PhysicalDevice PhysicalDevice;
        Vulkan::DeviceTable Table;

        u32 GraphicsCount = 0;
        u32 ComputeCount = 0;
        u32 TransferCount = 0;
        u32 PresentCount = 0;
    };

    /**
     * @brief Creates a Vulkan logical device with the specified settings.
     *
     * Configures the logical device using the provided physical device, queue priorities,
     * and any required features or extensions. Ensures compatibility with both the
     * Vulkan API and the physical device's capabilities.
     *
     * @param p_Instance The Vulkan instance associated with the logical device.
     * @param p_PhysicalDevice The physical device to base the logical device on.
     * @param p_QueuePriorities The queue priorities to assign to the device's queues.
     *
     * @return A `Result` containing the created `LogicalDevice` or an error if the creation fails.
     */
    static Result<LogicalDevice> Create(const Instance &p_Instance, const PhysicalDevice &p_PhysicalDevice,
                                        TKit::Span<const QueuePriorities> p_QueuePriorities);

    /**
     * @brief Creates a Vulkan logical device with the specified parameters.
     *
     * Configures the logical device using the given physical device, instance,
     * and any required features or extensions. Ensures the configuration is compatible
     * with the physical device and Vulkan API specifications.
     *
     * By default, the logical device will create one queue per type, each with a priority of 1.
     *
     * @param p_Instance The Vulkan instance associated with the logical device.
     * @param p_PhysicalDevice The physical device to use for creating the logical device.
     *
     * @return A `Result` containing the created `LogicalDevice` or an error if the creation fails.
     */
    static Result<LogicalDevice> Create(const Instance &p_Instance, const PhysicalDevice &p_PhysicalDevice);

    LogicalDevice() = default;
    LogicalDevice(const VkDevice p_Device, const Info &p_Info) : m_Device(p_Device), m_Info(p_Info)
    {
    }

    void Destroy();
    void SubmitForDeletion(DeletionQueue &p_Queue) const;

    VkDevice GetHandle() const
    {
        return m_Device;
    }

#ifdef VK_KHR_surface
    Result<PhysicalDevice::SwapChainSupportDetails> QuerySwapChainSupport(VkSurfaceKHR p_Surface) const;
#endif
    Result<VkFormat> FindSupportedFormat(TKit::Span<const VkFormat> p_Candidates, VkImageTiling p_Tiling,
                                         VkFormatFeatureFlags p_Features) const;

    static void WaitIdle(const Proxy &p_Device);
    void WaitIdle() const;

    Result<VkQueue> GetQueue(QueueType p_Type, u32 p_QueueIndex = 0) const;
    Result<VkQueue> GetQueue(u32 p_FamilyIndex, u32 p_QueueIndex = 0) const;

    Proxy CreateProxy() const;

    const Info &GetInfo() const
    {
        return m_Info;
    }

    operator VkDevice() const
    {
        return m_Device;
    }
    operator Proxy() const
    {
        return CreateProxy();
    }
    operator bool() const
    {
        return m_Device != VK_NULL_HANDLE;
    }

  private:
    VkDevice m_Device = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit
