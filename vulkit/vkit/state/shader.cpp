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

Result<Shader> Shader::Create(const LogicalDevice::Proxy &p_Device, const std::string_view p_SpirvPath)
{
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkCreateShaderModule, Result<Shader>);
    VKIT_CHECK_TABLE_FUNCTION_OR_RETURN(p_Device.Table, vkDestroyShaderModule, Result<Shader>);

    std::ifstream file{p_SpirvPath.data(), std::ios::ate | std::ios::binary};
    if (!file.is_open())
        return Result<Shader>::Error(
            VKIT_FORMAT_ERROR(VK_ERROR_INITIALIZATION_FAILED, "File at path '{}' not found", p_SpirvPath));

    const auto fileSize = file.tellg();

    TKit::Array<char, MaxShaderSize> code(static_cast<u32>(fileSize));
    file.seekg(0);
    file.read(code.GetData(), fileSize);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = fileSize;
    createInfo.pCode = reinterpret_cast<const u32 *>(code.GetData());

    VkShaderModule module;
    const VkResult result =
        p_Device.Table->CreateShaderModule(p_Device, &createInfo, p_Device.AllocationCallbacks, &module);
    if (result != VK_SUCCESS)
        return Result<Shader>::Error(result, "Failed to create shader module");

    return Result<Shader>::Ok(p_Device, module);
}

TKIT_COMPILER_WARNING_IGNORE_POP()

TKit::Result<void, i32> Shader::CompileFromFile(const std::string_view p_SourcePath, const std::string_view p_SpirvPath,
                                                const std::string_view p_Arguments)
{
    const fs::path spvPath = p_SpirvPath;

    std::filesystem::create_directories(spvPath.parent_path());
    const std::string compileCommand =
        VKIT_GLSL_BINARY " " + std::string(p_Arguments) + std::string(p_SourcePath) + " -o " + spvPath.string();
    const i32 code = static_cast<i32>(std::system(compileCommand.c_str()));
    if (code == 0)
        return TKit::Result<void, i32>::Ok();

    return code;
}

Result<Shader> Shader::CompileFromFile(const LogicalDevice::Proxy &p_Device, const std::string_view p_SourcePath,
                                       const std::string_view p_Arguments)
{
    const std::string spv = (fs::temp_directory_path() / "vkit-shader.spv").string();
    const auto result = CompileFromFile(p_SourcePath, spv, p_Arguments);
    if (!result)
        return Result<Shader>::Error(VKIT_FORMAT_ERROR(VK_ERROR_INITIALIZATION_FAILED,
                                                       "[VULKIT] Shader compilation failed with error code: {}",
                                                       result.GetError()));
    return Create(p_Device, spv);
}

Result<Shader> Shader::CompileFromSource(const LogicalDevice::Proxy &p_Device, const std::string_view p_SourceCode,
                                         const std::string_view p_Arguments)
{
    const fs::path src = fs::temp_directory_path() / "vkit-shader.glsl";
    std::ofstream file{src, std::ios::binary};
    file.write(p_SourceCode.data(), p_SourceCode.size());
    file.flush();

    return CompileFromFile(p_Device, src.string(), p_Arguments);
}

bool Shader::MustCompile(const std::string_view p_SourcePath, const std::string_view p_SpirvPath)
{
    const fs::path sourcePath = p_SourcePath;
    const fs::path spvPath = p_SpirvPath;

    TKIT_ASSERT(fs::exists(sourcePath), "[VULKIT] Source file does not exist");

    if (!fs::exists(spvPath))
        return true;

    const auto sourceTime = fs::last_write_time(sourcePath);
    const auto spvTime = fs::last_write_time(spvPath);
    return sourceTime > spvTime;
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
