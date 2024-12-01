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
    class Builder
    {
      public:
        explicit Builder(const Instance &p_Instance, const PhysicalDevice &p_PhysicalDevice) noexcept;

        Result<LogicalDevice> Build() const noexcept;

        Builder &SetQueuePriorities(std::span<const QueuePriorities> p_QueuePriorities) noexcept;

      private:
        Instance m_Instance;
        PhysicalDevice m_PhysicalDevice;

        // Will default to one queue per type with priority 1.0f
        DynamicArray<QueuePriorities> m_QueuePriorities;
    };

    void Destroy() noexcept;
    VkDevice GetDevice() const noexcept;

    VkQueue GetQueue(QueueType p_Type, u32 p_QueueIndex = 0) const noexcept;

    // This requires a call to vkGetDeviceQueue
    VkQueue GetQueue(u32 p_FamilyIndex, u32 p_QueueIndex = 0) const noexcept;

    explicit(false) operator VkDevice() const noexcept;
    explicit(false) operator bool() const noexcept;

    template <typename F> F GetFunction(const char *p_Name) const noexcept
    {
        return reinterpret_cast<F>(vkGetDeviceProcAddr(m_Device, p_Name));
    }

  private:
    using QueueArray = std::array<VkQueue, VKIT_MAX_QUEUES_PER_FAMILY>;

    LogicalDevice(VkDevice p_Device, const QueueArray &p_Queues) noexcept;

    VkDevice m_Device;
    QueueArray m_Queues;
};
} // namespace VKit