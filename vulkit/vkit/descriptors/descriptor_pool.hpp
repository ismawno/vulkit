#pragma once

#include "vkit/core/api.hpp"
#include "vkit/core/alias.hpp"
#include "vkit/backend/logical_device.hpp"
#include "tkit/profiling/vulkan.hpp"

#include <vulkan/vulkan.hpp>
#include <span>

namespace VKit
{
class VKIT_API DescriptorPool : public TKit::RefCounted<DescriptorPool>
{
  public:
    struct Specs
    {
        u32 MaxSets;
        std::span<const VkDescriptorPoolSize> PoolSizes;
        VkDescriptorPoolCreateFlags PoolFlags = 0;
    };

    DescriptorPool(const Specs &p_Specs) noexcept;
    ~DescriptorPool() noexcept;

    VkDescriptorSet Allocate(VkDescriptorSetLayout p_Layout) const noexcept;

    void Deallocate(std::span<const VkDescriptorSet> p_Sets) const noexcept;
    void Deallocate(VkDescriptorSet p_Set) const noexcept;
    void Reset() noexcept;

  private:
    LogicalDevice m_Device;
    VkDescriptorPool m_Pool;
};
} // namespace VKit