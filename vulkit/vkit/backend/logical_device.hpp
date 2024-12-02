#pragma once

#include "vkit/backend/physical_device.hpp"

#ifndef VKIT_MAX_QUEUES_PER_FAMILY
#    define VKIT_MAX_QUEUES_PER_FAMILY 16
#endif

namespace VKit
{
struct QueuePriorities
{
    u32 Index;
    DynamicArray<f32> Priorities;
};

enum class QueueType : u32
{
    Graphics = 0,
    Compute = 1,
    Transfer = 2,
    Present = 3
};

class LogicalDevice
{
  public:
    struct Proxy
    {
        VkDevice Device;
        const VkAllocationCallbacks *AllocationCallbacks;

        explicit(false) operator VkDevice() const noexcept;
    };

    class Builder
    {
      public:
        explicit Builder(const Instance *p_Instance, const PhysicalDevice *p_PhysicalDevice) noexcept;

        Result<LogicalDevice> Build() const noexcept;

        Builder &SetQueuePriorities(std::span<const QueuePriorities> p_QueuePriorities) noexcept;

      private:
        const Instance *m_Instance;
        const PhysicalDevice *m_PhysicalDevice;

        // Will default to one queue per type with priority 1.0f
        DynamicArray<QueuePriorities> m_QueuePriorities;
    };

    void Destroy() noexcept;

    const Instance &GetInstance() const noexcept;
    const PhysicalDevice &GetPhysicalDevice() const noexcept;
    VkDevice GetDevice() const noexcept;

    static void WaitIdle(VkDevice p_Device) noexcept;
    void WaitIdle() const noexcept;

    VkQueue GetQueue(QueueType p_Type, u32 p_QueueIndex = 0) const noexcept;

    // This requires a call to vkGetDeviceQueue
    VkQueue GetQueue(u32 p_FamilyIndex, u32 p_QueueIndex = 0) const noexcept;

    Proxy CreateProxy() const noexcept;

    explicit(false) operator VkDevice() const noexcept;
    explicit(false) operator bool() const noexcept;

    template <typename F> F GetFunction(const char *p_Name) const noexcept
    {
        return reinterpret_cast<F>(vkGetDeviceProcAddr(m_Device, p_Name));
    }

    void SubmitForDeletion(DeletionQueue &p_Queue) noexcept;

  private:
    using QueueArray = std::array<VkQueue, VKIT_MAX_QUEUES_PER_FAMILY>;

    LogicalDevice(const Instance &p_Instance, const PhysicalDevice &p_PhysicalDevice, VkDevice p_Device,
                  const QueueArray &p_Queues) noexcept;

    Instance m_Instance;
    PhysicalDevice m_PhysicalDevice;
    VkDevice m_Device;
    QueueArray m_Queues;
};
} // namespace VKit