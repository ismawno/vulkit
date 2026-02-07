#pragma once

#ifndef VKIT_ENABLE_DESCRIPTORS
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_DESCRIPTORS"
#endif

#include "vkit/device/proxy_device.hpp"
#include "vkit/state/descriptor_set.hpp"

#include <vulkan/vulkan.h>

namespace VKit
{
class DescriptorPool
{
  public:
    class Builder
    {
      public:
        Builder(const ProxyDevice &device) : m_Device(device)
        {
        }

        VKIT_NO_DISCARD Result<DescriptorPool> Build() const;

        Builder &SetMaxSets(u32 maxSets);
        Builder &SetFlags(VkDescriptorPoolCreateFlags flags);
        Builder &AddFlags(VkDescriptorPoolCreateFlags flags);
        Builder &RemoveFlags(VkDescriptorPoolCreateFlags flags);
        Builder &AddPoolSize(VkDescriptorType type, u32 size);

      private:
        ProxyDevice m_Device;

        u32 m_MaxSets = 8;
        VkDescriptorPoolCreateFlags m_Flags = 0;
        TKit::TierArray<VkDescriptorPoolSize> m_PoolSizes{};
    };

    struct Info
    {
        u32 MaxSets;
        TKit::TierArray<VkDescriptorPoolSize> PoolSizes;
    };

    DescriptorPool() = default;
    DescriptorPool(const ProxyDevice &device, const VkDescriptorPool pool, const Info &info)
        : m_Device(device), m_Pool(pool), m_Info(info)
    {
    }

    void Destroy();

    const Info &GetInfo() const
    {
        return m_Info;
    }

    VKIT_NO_DISCARD Result<DescriptorSet> Allocate(VkDescriptorSetLayout layout) const;
    VKIT_NO_DISCARD Result<> Deallocate(TKit::Span<const VkDescriptorSet> sets) const;
    VKIT_NO_DISCARD Result<> Reset(VkDescriptorPoolResetFlags flags = 0);

    const ProxyDevice &GetDevice() const
    {
        return m_Device;
    }
    VkDescriptorPool GetHandle() const
    {
        return m_Pool;
    }
    operator VkDescriptorPool() const
    {
        return m_Pool;
    }
    operator bool() const
    {
        return m_Pool != VK_NULL_HANDLE;
    }

  private:
    ProxyDevice m_Device{};
    VkDescriptorPool m_Pool = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit
