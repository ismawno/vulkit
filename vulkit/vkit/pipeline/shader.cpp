#include "vkit/core/pch.hpp"
#include "vkit/pipeline/shader.hpp"

#include <fstream>
#include <filesystem>
#include <cstdlib>

namespace VKit
{
FormattedResult<VkShaderModule> CreateShaderModule(const LogicalDevice &p_Device,
                                                   std::string_view p_BinaryPath) noexcept
{
    std::ifstream file{p_BinaryPath, std::ios::ate | std::ios::binary};
    if (!file.is_open())
        return FormattedResult<VkShaderModule>::Error(
            VKIT_FORMAT_ERROR(VK_ERROR_INITIALIZATION_FAILED, "File at path {} not found", p_BinaryPath));

    const auto fileSize = file.tellg();

    DynamicArray<char> code(fileSize);
    file.seekg(0);
    file.read(code.data(), fileSize);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = fileSize;
    createInfo.pCode = reinterpret_cast<const u32 *>(code.data());

    VkShaderModule module;
    const VkResult result = vkCreateShaderModule(p_Device.GetDevice(), &createInfo,
                                                 p_Device.GetInstance().GetInfo().AllocationCallbacks, &module);
    if (result != VK_SUCCESS)
        return FormattedResult<VkShaderModule>::Error(VKIT_FORMAT_ERROR(result, "Failed to create shader module"));

    return FormattedResult<VkShaderModule>::Ok(module);
}

void DestroyShaderModule(const LogicalDevice &p_Device, VkShaderModule p_ShaderModule) noexcept
{
    vkDestroyShaderModule(p_Device.GetDevice(), p_ShaderModule, p_Device.GetInstance().GetInfo().AllocationCallbacks);
}

void SubmitShaderModuleForDeletion(const LogicalDevice &p_Device, DeletionQueue &p_Queue,
                                   VkShaderModule p_ShaderModule) noexcept
{
    const VkDevice device = p_Device.GetDevice();
    const VkAllocationCallbacks *allocationCallbacks = p_Device.GetInstance().GetInfo().AllocationCallbacks;
    p_Queue.Push([device, allocationCallbacks, p_ShaderModule]() {
        vkDestroyShaderModule(device, p_ShaderModule, allocationCallbacks);
    });
}

i32 CompileShader(std::string_view p_SourcePath, std::string_view p_BinaryPath) noexcept
{
    namespace fs = std::filesystem;
    const fs::path binaryPath = p_BinaryPath;

    std::filesystem::create_directories(binaryPath.parent_path());

    const std::string compileCommand = VKIT_GLSL_BINARY " " + std::string(p_SourcePath) + " -o " + binaryPath.string();

    const i32 result = std::system(compileCommand.c_str());
    if (result != 0)
        return result;

    return result;
}

} // namespace VKit