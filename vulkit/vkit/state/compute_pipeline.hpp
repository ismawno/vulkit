#pragma once

#ifndef VKIT_ENABLE_COMPUTE_PIPELINE
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_COMPUTE_PIPELINE"
#endif

#include "vkit/device/logical_device.hpp"

namespace VKit
{
class VKIT_API ComputePipeline
{
  public:
    struct Specs
    {
        VkPipelineLayout Layout = VK_NULL_HANDLE;
        VkShaderModule ComputeShader = VK_NULL_HANDLE;
        const char *EntryPoint = "main";
        VkPipelineCache Cache = VK_NULL_HANDLE;
    };

    static Result<ComputePipeline> Create(const LogicalDevice::Proxy &p_Device, const Specs &p_Specs);

    static Result<> Create(const LogicalDevice::Proxy &p_Device, TKit::Span<const Specs> p_Specs,
                           TKit::Span<ComputePipeline> p_Pipelines, VkPipelineCache p_Cache = VK_NULL_HANDLE);

    ComputePipeline() = default;
    ComputePipeline(const LogicalDevice::Proxy &p_Device, VkPipeline p_Pipeline)
        : m_Device(p_Device), m_Pipeline(p_Pipeline)
    {
    }

    void Destroy();

    void Bind(VkCommandBuffer p_CommandBuffer) const;

    const LogicalDevice::Proxy &GetDevice() const
    {
        return m_Device;
    }
    VkPipeline GetHandle() const
    {
        return m_Pipeline;
    }
    operator VkPipeline() const
    {
        return m_Pipeline;
    }
    operator bool() const
    {
        return m_Pipeline != VK_NULL_HANDLE;
    }

  private:
    LogicalDevice::Proxy m_Device{};
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
};
} // namespace VKit
