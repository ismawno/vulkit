#pragma once

#include "vkit/backend/logical_device.hpp"
#include <vulkan/vulkan.hpp>

namespace VKit
{
class VKIT_API Shader
{
  public:
    static FormattedResult<Shader> Create(const LogicalDevice::Proxy &p_Device, std::string_view p_BinaryPath) noexcept;
    static i32 Compile(std::string_view p_SourcePath, std::string_view p_BinaryPath) noexcept;

    Shader() noexcept = default;
    Shader(const LogicalDevice::Proxy &p_Device, VkShaderModule p_Module) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

    VkShaderModule GetModule() const noexcept;
    explicit(false) operator VkShaderModule() const noexcept;
    explicit(false) operator bool() const noexcept;

  private:
    LogicalDevice::Proxy m_Device{};
    VkShaderModule m_Module = VK_NULL_HANDLE;
};
} // namespace VKit