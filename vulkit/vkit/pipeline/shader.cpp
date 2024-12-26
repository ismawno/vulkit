#include "vkit/core/pch.hpp"
#include "vkit/pipeline/shader.hpp"

#include <fstream>
#include <filesystem>
#include <cstdlib>

namespace VKit
{
Shader::Shader(const LogicalDevice::Proxy &p_Device, VkShaderModule p_Module) noexcept
    : m_Device(p_Device), m_Module(p_Module)
{
}

FormattedResult<Shader> Shader::Create(const LogicalDevice::Proxy &p_Device,
                                       const std::string_view p_BinaryPath) noexcept
{
    std::ifstream file{p_BinaryPath.data(), std::ios::ate | std::ios::binary};
    if (!file.is_open())
        return FormattedResult<Shader>::Error(
            VKIT_FORMAT_ERROR(VK_ERROR_INITIALIZATION_FAILED, "File at path {} not found", p_BinaryPath));

    const auto fileSize = file.tellg();

    TKit::StaticArray<char, VKIT_MAX_SHADER_SIZE> code(fileSize);
    file.seekg(0);
    file.read(code.data(), fileSize);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = fileSize;
    createInfo.pCode = reinterpret_cast<const u32 *>(code.data());

    VkShaderModule module;
    const VkResult result = vkCreateShaderModule(p_Device, &createInfo, p_Device.AllocationCallbacks, &module);
    if (result != VK_SUCCESS)
        return FormattedResult<Shader>::Error(result, "Failed to create shader module");

    return FormattedResult<Shader>::Ok(p_Device, module);
}

i32 Shader::Compile(const std::string_view p_SourcePath, const std::string_view p_BinaryPath) noexcept
{
    namespace fs = std::filesystem;
    const fs::path binaryPath = p_BinaryPath;

    std::filesystem::create_directories(binaryPath.parent_path());
    const std::string compileCommand = VKIT_GLSL_BINARY " " + std::string(p_SourcePath) + " -o " + binaryPath.string();
    return std::system(compileCommand.c_str());
}

i32 Shader::CompileIfModified(const std::string_view p_SourcePath, const std::string_view p_BinaryPath) noexcept
{
    namespace fs = std::filesystem;
    const fs::path sourcePath = p_SourcePath;
    const fs::path binaryPath = p_BinaryPath;

    if (!fs::exists(sourcePath))
        return 1;

    if (!fs::exists(binaryPath))
        return Compile(p_SourcePath, p_BinaryPath);

    const auto sourceTime = fs::last_write_time(sourcePath);
    const auto binaryTime = fs::last_write_time(binaryPath);

    if (sourceTime > binaryTime)
        return Compile(p_SourcePath, p_BinaryPath);

    return INT32_MAX;
}

void Shader::Destroy() noexcept
{
    TKIT_ASSERT(m_Module, "The shader is a NULL handle");
    vkDestroyShaderModule(m_Device, m_Module, m_Device.AllocationCallbacks);
    m_Module = VK_NULL_HANDLE;
}
void Shader::SubmitForDeletion(DeletionQueue &p_Queue) const noexcept
{
    const VkShaderModule module = m_Module;
    const LogicalDevice::Proxy device = m_Device;
    p_Queue.Push([module, device]() { vkDestroyShaderModule(device, module, device.AllocationCallbacks); });
}

VkShaderModule Shader::GetModule() const noexcept
{
    return m_Module;
}
Shader::operator VkShaderModule() const noexcept
{
    return m_Module;
}
Shader::operator bool() const noexcept
{
    return m_Module != VK_NULL_HANDLE;
}

} // namespace VKit