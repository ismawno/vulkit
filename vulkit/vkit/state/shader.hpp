#pragma once

#ifndef VKIT_ENABLE_SHADERS
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_SHADERS"
#endif

#include "vkit/core/alias.hpp"
#include "vkit/device/proxy_device.hpp"
#include <vulkan/vulkan.h>

namespace VKit
{
class Shader
{
  public:
    static Result<Shader> Create(const ProxyDevice &p_Device, std::string_view p_SpirvPath);
    static Result<Shader> Create(const ProxyDevice &p_Device, const u32 *p_Spirv);

    static TKit::Result<void, i32> CompileFromFile(std::string_view p_SourcePath, std::string_view p_SpirvPath,
                                                   std::string_view p_Arguments = "");

    static Result<Shader> CompileFromFile(const ProxyDevice &p_Device, std::string_view p_SourcePath,
                                          std::string_view p_Arguments = "");

    static Result<Shader> CompileFromSource(const ProxyDevice &p_Device, std::string_view p_SourceCode,
                                            std::string_view p_Arguments = "");

    static bool MustCompile(std::string_view p_SourcePath, std::string_view p_SpirvPath);

    Shader() = default;
    Shader(const ProxyDevice &p_Device, VkShaderModule p_Module) : m_Device(p_Device), m_Module(p_Module)
    {
    }

    void Destroy();

    const ProxyDevice &GetDevice() const
    {
        return m_Device;
    }
    VkShaderModule GetHandle() const
    {
        return m_Module;
    }
    operator VkShaderModule() const
    {
        return m_Module;
    }
    operator bool() const
    {
        return m_Module != VK_NULL_HANDLE;
    }

  private:
    ProxyDevice m_Device{};
    VkShaderModule m_Module = VK_NULL_HANDLE;
};
} // namespace VKit
