#include "vkit/core/pch.hpp"
#include "vkit/state/shader.hpp"
#include "vkit/core/limits.hpp"

#include <fstream>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

namespace VKit
{

TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_MSVC_WARNING_IGNORE(6262)

Result<Shader> Shader::Create(const ProxyDevice &p_Device, const std::string_view p_SpirvPath)
{
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkCreateShaderModule, Result<Shader>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkDestroyShaderModule, Result<Shader>);

    std::ifstream file{p_SpirvPath.data(), std::ios::ate | std::ios::binary};
    if (!file.is_open())
        return Result<Shader>::Error(Error_FileNotFound, TKit::Format("File at path '{}' not found", p_SpirvPath));

    const auto fileSize = file.tellg();

    TKit::Array<char, MaxShaderSize> code(static_cast<u32>(fileSize));
    file.seekg(0);
    file.read(code.GetData(), fileSize);
    const u32 *spv = reinterpret_cast<const u32 *>(code.GetData());
    return Create(p_Device, spv, fileSize);
}

TKIT_COMPILER_WARNING_IGNORE_POP()

Result<Shader> Shader::Create(const ProxyDevice &p_Device, const u32 *p_Spirv, const size_t p_Size)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = p_Size;
    createInfo.pCode = p_Spirv;

    VkShaderModule module;
    const VkResult result =
        p_Device.Table->CreateShaderModule(p_Device, &createInfo, p_Device.AllocationCallbacks, &module);
    if (result != VK_SUCCESS)
        return Result<Shader>::Error(result);

    return Result<Shader>::Ok(p_Device, module);
}

void Shader::Destroy()
{
    if (m_Module)
    {
        m_Device.Table->DestroyShaderModule(m_Device, m_Module, m_Device.AllocationCallbacks);
        m_Module = VK_NULL_HANDLE;
    }
}
} // namespace VKit
