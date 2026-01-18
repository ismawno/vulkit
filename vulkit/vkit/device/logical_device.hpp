#pragma once

#ifndef VKIT_ENABLE_LOGICAL_DEVICE
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_LOGICAL_DEVICE"
#endif

#include "vkit/device/physical_device.hpp"
#include "vkit/execution/queue.hpp"

namespace VKit
{
class LogicalDevice
{
  public:
    struct QueuePriorities
    {
        TKit::TierArray<f32> RequiredPriorities;
        TKit::TierArray<f32> RequestedPriorities;
    };

    class Builder
    {
      public:
        Builder(Instance *p_Instance, PhysicalDevice *p_PhysicalDevice)
            : m_Instance(p_Instance), m_PhysicalDevice(p_PhysicalDevice),
              m_Priorities(p_PhysicalDevice->GetInfo().QueueFamilies.GetSize())
        {
        }

        VKIT_NO_DISCARD Result<LogicalDevice> Build() const;

        Builder &RequireQueue(QueueType p_Type, u32 p_Count = 1, f32 p_Priority = 1.f);
        Builder &RequestQueue(QueueType p_Type, u32 p_Count = 1, f32 p_Priority = 1.f);

        Builder &RequireQueue(u32 p_Family, u32 p_Count = 1, f32 p_Priority = 1.f);
        Builder &RequestQueue(u32 p_Family, u32 p_Count = 1, f32 p_Priority = 1.f);

      private:
        Instance *m_Instance;
        PhysicalDevice *m_PhysicalDevice;

        TKit::TierArray<QueuePriorities> m_Priorities;
    };

    struct Info
    {
        Instance *Instance;
        PhysicalDevice *PhysicalDevice;
        Vulkan::DeviceTable Table;
        TKit::TierArray<Queue *> Queues;
        TKit::FixedArray<TKit::TierArray<Queue *>, Queue_Count> QueuesPerType;
    };

    LogicalDevice() = default;
    LogicalDevice(const VkDevice p_Device, const Info &p_Info) : m_Device(p_Device), m_Info(p_Info)
    {
    }

    void Destroy();

    VkDevice GetHandle() const
    {
        return m_Device;
    }

#ifdef VK_KHR_surface
    VKIT_NO_DISCARD Result<PhysicalDevice::SwapChainSupportDetails> QuerySwapChainSupport(VkSurfaceKHR p_Surface) const;
#endif
    VKIT_NO_DISCARD Result<VkFormat> FindSupportedFormat(TKit::Span<const VkFormat> p_Candidates,
                                                         VkImageTiling p_Tiling, VkFormatFeatureFlags p_Features) const;

    VKIT_NO_DISCARD static Result<> WaitIdle(const ProxyDevice &p_Device);
    VKIT_NO_DISCARD Result<> WaitIdle() const;

    ProxyDevice CreateProxy() const;

    const Info &GetInfo() const
    {
        return m_Info;
    }

    operator VkDevice() const
    {
        return m_Device;
    }
    operator ProxyDevice() const
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
