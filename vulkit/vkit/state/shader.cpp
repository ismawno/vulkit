#include "vkit/core/pch.hpp"
#include "vkit/state/shader.hpp"
#include "tkit/container/stack_array.hpp"

#include <fstream>
#include <cstdlib>

namespace VKit
{

TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_MSVC_WARNING_IGNORE(6262)

Result<Shader> Shader::Create(const ProxyDevice &device, const std::string_view spirvPath)
{
    std::ifstream file{spirvPath.data(), std::ios::ate | std::ios::binary};
    if (!file.is_open())
        return Result<Shader>::Error(Error_FileNotFound,
                                     TKit::Format("[VULKIT][SHADER] File at path '{}' not found", spirvPath));

    const auto fileSize = file.tellg();

    TKit::StackArray<char> code{static_cast<u32>(fileSize)};
    file.seekg(0);
    file.read(code.GetData(), fileSize);
    const u32 *spv = reinterpret_cast<const u32 *>(code.GetData());
    return Create(device, spv, fileSize);
}

TKIT_COMPILER_WARNING_IGNORE_POP()

Result<Shader> Shader::Create(const ProxyDevice &device, const u32 *spirv, const size_t size)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    createInfo.pCode = spirv;

    VkShaderModule module;
    const VkResult result = device.Table->CreateShaderModule(device, &createInfo, device.AllocationCallbacks, &module);
    if (result != VK_SUCCESS)
        return Result<Shader>::Error(result);

    return Result<Shader>::Ok(device, module);
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
