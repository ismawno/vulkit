#pragma once

#include "vkit/backend/physical_device.hpp"

namespace VKit
{
struct QueuePriorities
{
    u32 Index;
    DynamicArray<f32> Priorities;
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
    VkInstance GetDevice() const noexcept;

    explicit(false) operator VkDevice() const noexcept;
    explicit(false) operator bool() const noexcept;

    template <typename F> F GetFunction(const char *p_Name) const noexcept
    {
        return reinterpret_cast<F>(vkGetDeviceProcAddr(m_Device, p_Name));
    }

  private:
    LogicalDevice(VkDevice p_Device) noexcept;

    VkDevice m_Device;
};
} // namespace VKit