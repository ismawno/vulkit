#include "vkit/core/pch.hpp"
#include "vkit/state/shader.hpp"
#include "tkit/container/stack_array.hpp"
#include "tkit/filesystem/file.hpp"

namespace VKit
{
TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_MSVC_WARNING_IGNORE(6262)

Result<Shader> Shader::Create(const ProxyDevice &device, const TKit::StringView spirvPath)
{
    const TKit::StackPath path = spirvPath;
    const auto res = TKit::ReadFile<TKit::StackString>(path);
    VKIT_RETURN_ERROR_IF(!res, Error_FileNotFound, "[VULKIT][SHADER] Failed to load shader at '{}' with error: {}",
                         path, TKit::ToString(res.GetError()));

    const TKit::StackString &code = *res;
    const u32 *spv = rcast<const u32 *>(code.GetData());
    return Create(device, spv, code.GetBytes());
}

TKIT_COMPILER_WARNING_IGNORE_POP()

Result<Shader> Shader::Create(const ProxyDevice &device, const u32 *spirv, const usz size)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    createInfo.pCode = spirv;

    VkShaderModule module;
    VKIT_RETURN_IF_FAILED(device.Table->CreateShaderModule(device, &createInfo, device.AllocationCallbacks, &module));

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
