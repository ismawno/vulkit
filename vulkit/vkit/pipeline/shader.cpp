#include "vkit/core/pch.hpp"
#include "vkit/pipeline/shader.hpp"
#include "vkit/core/core.hpp"
#include "tkit/memory/stack_allocator.hpp"

#include <fstream>
#include <filesystem>
#include <cstdlib>

namespace VKit
{
Shader::Shader(const std::string_view p_BinaryPath) noexcept
{
    m_Device = Core::GetDevice();
    TKIT_ASSERT(
        m_Device,
        "A shader requires an already initialized device, which in turns needs a first window to be already created.");

    createShaderModule(p_BinaryPath);
}

Shader::Shader(const std::string_view p_SourcePath, const std::string_view p_BinaryPath) noexcept
{
    m_Device = Core::GetDevice();
    TKIT_ASSERT(
        m_Device,
        "A shader requires an already initialized device, which in turns needs a first window to be already created.");

    if (!std::filesystem::exists(p_BinaryPath))
        compileShader(p_SourcePath, p_BinaryPath);
    createShaderModule(p_BinaryPath);
}

Shader::~Shader() noexcept
{
    vkDestroyShaderModule(m_Device->GetDevice(), m_Module, nullptr);
}

VkShaderModule Shader::GetModule() const noexcept
{
    return m_Module;
}

void Shader::compileShader(const std::string_view p_SourcePath, const std::string_view p_BinaryPath) noexcept
{
    namespace fs = std::filesystem;
    const fs::path binaryPath = p_BinaryPath;

    std::filesystem::create_directories(binaryPath.parent_path());

    const std::string compileCommand = VKIT_GLSL_BINARY " " + std::string(p_SourcePath) + " -o " + binaryPath.string();

    const i32 result = std::system(compileCommand.c_str());
    TKIT_ASSERT(result == 0, "Failed to compile shader at path: {}", p_SourcePath);
    if (result != 0)
        std::terminate();

    TKIT_LOG_INFO("Compiled shader at: {}", p_SourcePath);
}

void Shader::createShaderModule(const std::string_view p_Path) noexcept
{
    std::ifstream file{p_Path, std::ios::ate | std::ios::binary};
    TKIT_ASSERT(file.is_open(), "File at path {} not found", p_Path);
    const auto fileSize = file.tellg();

    TKit::StackAllocator *allocator = Core::GetStackAllocator();

    char *code = static_cast<char *>(allocator->Push(fileSize * sizeof(char), alignof(u32)));
    file.seekg(0);
    file.read(code, fileSize);

    // TKIT_LOG_INFO("Creating shader module from file: {} with size: {}", p_Path, static_cast<usize>(fileSize));

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = fileSize;
    createInfo.pCode = reinterpret_cast<const u32 *>(code);

    TKIT_ASSERT_RETURNS(vkCreateShaderModule(m_Device->GetDevice(), &createInfo, nullptr, &m_Module), VK_SUCCESS,
                        "Failed to create shader module");

    allocator->Pop();
}

} // namespace VKit