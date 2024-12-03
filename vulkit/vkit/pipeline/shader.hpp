#pragma once

#include "vkit/backend/logical_device.hpp"
#include <vulkan/vulkan.hpp>

namespace VKit
{
class Shader
{
  public:
    static FormattedResult<Shader> Create(const LogicalDevice::Proxy &p_Device, std::string_view p_BinaryPath) noexcept;
    static i32 Compile(std::string_view p_SourcePath, std::string_view p_BinaryPath) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) noexcept;

    VkShaderModule GetModule() const noexcept;
    explicit(false) operator VkShaderModule() const noexcept;
    explicit(false) operator bool() const noexcept;

  private:
    Shader(const LogicalDevice::Proxy &p_Device, VkShaderModule p_Module) noexcept;

    LogicalDevice::Proxy m_Device;
    VkShaderModule m_Module;
};
} // namespace VKit