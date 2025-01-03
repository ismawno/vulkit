#pragma once

#include "vkit/backend/logical_device.hpp"
#include <vulkan/vulkan.h>

#ifndef VKIT_MAX_SHADER_SIZE
#    define VKIT_MAX_SHADER_SIZE 128 * 1024
#endif

namespace VKit
{
/**
 * @brief Represents a Vulkan shader module.
 *
 * Manages the creation, destruction, and usage of shader modules in Vulkan.
 * Provides utility methods for compiling shaders and loading precompiled binaries.
 */
class VKIT_API Shader
{
  public:
    /**
     * @brief Creates a shader module from a precompiled binary file.
     *
     * Loads the binary file from the specified path and initializes the shader module.
     *
     * @param p_Device The logical device proxy for Vulkan operations.
     * @param p_BinaryPath The path to the precompiled shader binary.
     * @return A result containing the created Shader or an error.
     */
    static FormattedResult<Shader> Create(const LogicalDevice::Proxy &p_Device, std::string_view p_BinaryPath) noexcept;

    /**
     * @brief Compiles a shader source file into a binary file.
     *
     * Invokes the Vulkan SDK shader compiler to compile the source code into SPIR-V format.
     *
     * @param p_SourcePath The path to the shader source file.
     * @param p_BinaryPath The path where the compiled binary will be saved.
     * @return An integer status code (0 for success, non-zero for failure).
     */
    static i32 Compile(std::string_view p_SourcePath, std::string_view p_BinaryPath) noexcept;

    /**
     * @brief Compiles a shader source file into a binary file if the source has been modified.
     *
     * Compares the modification times of the source and binary files to determine if recompilation is necessary.
     *
     * @param p_SourcePath The path to the shader source file.
     * @param p_BinaryPath The path where the compiled binary will be saved.
     * @return An integer status code (0 for success, non-zero for failure). `INT32_MAX` if the binary is up-to-date.
     */
    static i32 CompileIfModified(std::string_view p_SourcePath, std::string_view p_BinaryPath) noexcept;

    Shader() noexcept = default;
    Shader(const LogicalDevice::Proxy &p_Device, VkShaderModule p_Module) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

    VkShaderModule GetModule() const noexcept;
    explicit(false) operator VkShaderModule() const noexcept;
    explicit(false) operator bool() const noexcept;

  private:
    LogicalDevice::Proxy m_Device{};
    VkShaderModule m_Module = VK_NULL_HANDLE;
};
} // namespace VKit