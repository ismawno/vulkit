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
    VKIT_NO_DISCARD static Result<Shader> Create(const ProxyDevice &device, std::string_view spirvPath);
    VKIT_NO_DISCARD static Result<Shader> Create(const ProxyDevice &device, const u32 *spirv, size_t size);

    Shader() = default;
    Shader(const ProxyDevice &device, VkShaderModule module) : m_Device(device), m_Module(module)
    {
    }

    void Destroy();
    VKIT_SET_DEBUG_NAME(m_Module, VK_OBJECT_TYPE_SHADER_MODULE)

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
