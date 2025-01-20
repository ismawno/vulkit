#pragma once

#include "vkit/backend/physical_device.hpp"

#ifndef VKIT_MAX_QUEUES_PER_FAMILY
#    define VKIT_MAX_QUEUES_PER_FAMILY 4
#endif

namespace VKit
{
/**
 * @brief Defines the priorities for device queues.
 *
 * The amount of queues will be determined by the number of priorities provided.
 */
struct VKIT_API QueuePriorities
{
    u32 Index;
    TKit::StaticArray8<f32> Priorities;
};

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

        explicit(false) operator VkDevice() const noexcept;
        explicit(false) operator bool() const noexcept;
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
                                        TKit::Span<const QueuePriorities> p_QueuePriorities) noexcept;

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
    static Result<LogicalDevice> Create(const Instance &p_Instance, const PhysicalDevice &p_PhysicalDevice) noexcept;

    LogicalDevice() noexcept = default;
    LogicalDevice(const Instance &p_Instance, const PhysicalDevice &p_PhysicalDevice, VkDevice p_Device) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

    const Instance &GetInstance() const noexcept;
    const PhysicalDevice &GetPhysicalDevice() const noexcept;
    VkDevice GetDevice() const noexcept;

    Result<PhysicalDevice::SwapChainSupportDetails> QuerySwapChainSupport(VkSurfaceKHR p_Surface) const noexcept;
    Result<VkFormat> FindSupportedFormat(TKit::Span<const VkFormat> p_Candidates, VkImageTiling p_Tiling,
                                         VkFormatFeatureFlags p_Features) const noexcept;

    static void WaitIdle(VkDevice p_Device) noexcept;
    void WaitIdle() const noexcept;

    VkQueue GetQueue(QueueType p_Type, u32 p_QueueIndex = 0) const noexcept;
    VkQueue GetQueue(u32 p_FamilyIndex, u32 p_QueueIndex = 0) const noexcept;

    Proxy CreateProxy() const noexcept;

    explicit(false) operator VkDevice() const noexcept;
    explicit(false) operator Proxy() const noexcept;
    explicit(false) operator bool() const noexcept;

    template <typename F> F GetFunction(const char *p_Name) const noexcept
    {
        return reinterpret_cast<F>(vkGetDeviceProcAddr(m_Device, p_Name));
    }

  private:
    Instance m_Instance{};
    PhysicalDevice m_PhysicalDevice{};
    VkDevice m_Device = VK_NULL_HANDLE;
};
} // namespace VKit