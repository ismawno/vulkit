#pragma once

#ifndef VKIT_ENABLE_COMPUTE_PIPELINE
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_COMPUTE_PIPELINE"
#endif

#include "vkit/device/logical_device.hpp"

namespace VKit
{
class ComputePipeline
{
  public:
    struct Specs
    {
        VkPipelineLayout Layout = VK_NULL_HANDLE;
        VkShaderModule ComputeShader = VK_NULL_HANDLE;
        const char *EntryPoint = "main";
        VkPipelineCache Cache = VK_NULL_HANDLE;
    };

    static Result<ComputePipeline> Create(const ProxyDevice &device, const Specs &specs);

    static Result<> Create(const ProxyDevice &device, TKit::Span<const Specs> specs,
                           TKit::Span<ComputePipeline> pipelines, VkPipelineCache cache = VK_NULL_HANDLE);

    ComputePipeline() = default;
    ComputePipeline(const ProxyDevice &device, VkPipeline pipeline) : m_Device(device), m_Pipeline(pipeline)
    {
    }

    void Destroy();

    void Bind(VkCommandBuffer commandBuffer) const;

    const ProxyDevice &GetDevice() const
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
    ProxyDevice m_Device{};
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
};
} // namespace VKit
