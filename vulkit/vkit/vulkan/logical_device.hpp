#pragma once

#ifndef VKIT_ENABLE_LOGICAL_DEVICE
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_LOGICAL_DEVICE"
#endif

#include "vkit/vulkan/physical_device.hpp"

namespace VKit
{
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

    struct QueuePriorities
    {
        TKit::Array16<f32> RequiredPriorities;
        TKit::Array16<f32> RequestedPriorities;
    };

    class Builder
    {
      public:
        Builder(const Instance *p_Instance, const PhysicalDevice *p_PhysicalDevice)
            : m_Instance(p_Instance), m_PhysicalDevice(p_PhysicalDevice),
              m_Priorities(p_PhysicalDevice->GetInfo().QueueFamilies.GetSize())
        {
        }

        Result<LogicalDevice> Build() const;

        Builder &RequireQueue(QueueType p_Type, u32 p_Count = 1, f32 p_Priority = 1.f);
        Builder &RequestQueue(QueueType p_Type, u32 p_Count = 1, f32 p_Priority = 1.f);

        Builder &RequireQueue(u32 p_Family, u32 p_Count = 1, f32 p_Priority = 1.f);
        Builder &RequestQueue(u32 p_Family, u32 p_Count = 1, f32 p_Priority = 1.f);

      private:
        const Instance *m_Instance;
        const PhysicalDevice *m_PhysicalDevice;

        TKit::Array8<QueuePriorities> m_Priorities;
    };

    struct Info
    {
        Instance Instance;
        PhysicalDevice PhysicalDevice;
        Vulkan::DeviceTable Table;
        TKit::FixedArray4<u32> QueueCounts;
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
    Result<PhysicalDevice::SwapChainSupportDetails> QuerySwapChainSupport(VkSurfaceKHR p_Surface) const;
#endif
    Result<VkFormat> FindSupportedFormat(TKit::Span<const VkFormat> p_Candidates, VkImageTiling p_Tiling,
                                         VkFormatFeatureFlags p_Features) const;

    static Result<> WaitIdle(const Proxy &p_Device);
    Result<> WaitIdle() const;

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
