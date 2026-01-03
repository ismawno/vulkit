#pragma once

#ifndef VKIT_ENABLE_DESCRIPTORS
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_DESCRIPTORS"
#endif

#include "vkit/device/logical_device.hpp"
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
        Builder(const ProxyDevice &p_Device) : m_Device(p_Device)
        {
        }

        Result<DescriptorPool> Build() const;

        Builder &SetMaxSets(u32 p_MaxSets);
        Builder &SetFlags(VkDescriptorPoolCreateFlags p_Flags);
        Builder &AddFlags(VkDescriptorPoolCreateFlags p_Flags);
        Builder &RemoveFlags(VkDescriptorPoolCreateFlags p_Flags);
        Builder &AddPoolSize(VkDescriptorType p_Type, u32 p_Size);

      private:
        ProxyDevice m_Device;

        u32 m_MaxSets = 8;
        VkDescriptorPoolCreateFlags m_Flags = 0;
        TKit::Array32<VkDescriptorPoolSize> m_PoolSizes{};
    };

    struct Info
    {
        u32 MaxSets;
        TKit::Array32<VkDescriptorPoolSize> PoolSizes;
    };

    DescriptorPool() = default;
    DescriptorPool(const ProxyDevice &p_Device, const VkDescriptorPool p_Pool, const Info &p_Info)
        : m_Device(p_Device), m_Pool(p_Pool), m_Info(p_Info)
    {
    }

    void Destroy();

    const Info &GetInfo() const
    {
        return m_Info;
    }

    Result<DescriptorSet> Allocate(VkDescriptorSetLayout p_Layout) const;
    Result<> Deallocate(TKit::Span<const VkDescriptorSet> p_Sets) const;
    Result<> Deallocate(VkDescriptorSet p_Set) const;
    Result<> Reset();

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
