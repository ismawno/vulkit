#pragma once

#include "vkit/core/api.hpp"
#include "vkit/core/alias.hpp"
#include "vkit/backend/logical_device.hpp"
#include "tkit/profiling/vulkan.hpp"

#include <vulkan/vulkan.hpp>
#include <span>

namespace VKit
{
class VKIT_API DescriptorPool
{
  public:
    struct Info
    {
        u32 MaxSets;
        DynamicArray<VkDescriptorPoolSize> PoolSizes;
    };

    class Builder
    {
      public:
        explicit Builder(const LogicalDevice::Proxy &p_Device) noexcept;

        Result<DescriptorPool> Build() const noexcept;

        Builder &SetMaxSets(u32 p_MaxSets) noexcept;
        Builder &SetFlags(VkDescriptorPoolCreateFlags p_Flags) noexcept;
        Builder &AddFlags(VkDescriptorPoolCreateFlags p_Flags) noexcept;
        Builder &RemoveFlags(VkDescriptorPoolCreateFlags p_Flags) noexcept;
        Builder &AddPoolSize(VkDescriptorType p_Type, u32 p_Size) noexcept;

      private:
        LogicalDevice::Proxy m_Device;

        u32 m_MaxSets = 8;
        VkDescriptorPoolCreateFlags m_Flags = 0;
        DynamicArray<VkDescriptorPoolSize> m_PoolSizes{};
    };

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) noexcept;

    const Info &GetInfo() const noexcept;

    Result<VkDescriptorSet> Allocate(VkDescriptorSetLayout p_Layout) const noexcept;

    void Deallocate(std::span<const VkDescriptorSet> p_Sets) const noexcept;
    void Deallocate(VkDescriptorSet p_Set) const noexcept;
    void Reset() noexcept;

    VkDescriptorPool GetPool() const noexcept;
    explicit(false) operator VkDescriptorPool() const noexcept;
    explicit(false) operator bool() const noexcept;

  private:
    DescriptorPool(const LogicalDevice::Proxy &p_Device, VkDescriptorPool p_Pool, const Info &p_Info) noexcept;

    LogicalDevice::Proxy m_Device;
    VkDescriptorPool m_Pool;
    Info m_Info;
};
} // namespace VKit