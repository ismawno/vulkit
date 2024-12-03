#pragma once

#include "vkit/backend/logical_device.hpp"
#include <vulkan/vulkan.hpp>

namespace VKit
{
FormattedResult<VkShaderModule> CreateShaderModule(const LogicalDevice &p_Device,
                                                   std::string_view p_BinaryPath) noexcept;
void DestroyShaderModule(const LogicalDevice &p_Device, VkShaderModule p_ShaderModule) noexcept;
void SubmitShaderModuleForDeletion(const LogicalDevice &p_Device, DeletionQueue &p_Queue,
                                   VkShaderModule p_ShaderModule) noexcept;

i32 CompileShader(std::string_view p_SourcePath, std::string_view p_BinaryPath) noexcept;
} // namespace VKit