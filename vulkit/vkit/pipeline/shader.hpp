#pragma once

#ifndef VKIT_ENABLE_SHADERS
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_SHADERS"
#endif

#include "vkit/vulkan/logical_device.hpp"
#include <vulkan/vulkan.h>

#ifndef VKIT_MAX_SHADER_SIZE
#    define VKIT_MAX_SHADER_SIZE 128 * 1024
#endif

#if VKIT_MAX_SHADER_SIZE < 1
#    error "[VULKIT] Maximum shader size must be greater than 0"
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
     * @return A `Result` containing the created `Shader` or an error.
     */
    static FormattedResult<Shader> Create(const LogicalDevice::Proxy &p_Device, std::string_view p_BinaryPath) noexcept;

    /**
     * @brief Compiles a shader source file into a binary file.
     *
     * Invokes the Vulkan SDK shader compiler to compile the source code into SPIR-V format.
     *
     * @param p_SourcePath The path to the shader source file.
     * @param p_BinaryPath The path where the compiled binary will be saved.
     * @param p_Arguments Additional arguments to pass to the compiler.
     * @return An integer status code (0 for success, non-zero for failure).
     */
    static i32 Compile(std::string_view p_SourcePath, std::string_view p_BinaryPath,
                       std::string_view p_Arguments = "") noexcept;

    /**
     * @brief Determines if a shader source file must be compiled.
     *
     * Compares the modification times of the source and binary files to determine if recompilation is necessary.
     *
     * @param p_SourcePath The path to the shader source file.
     * @param p_BinaryPath The path where the compiled binary will be saved.
     * @return `true` if the source must be compiled, `false` if the binary is up-to-date.
     */
    static bool MustCompile(std::string_view p_SourcePath, std::string_view p_BinaryPath) noexcept;

    Shader() noexcept = default;
    Shader(const LogicalDevice::Proxy &p_Device, VkShaderModule p_Module) noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

    const LogicalDevice::Proxy &GetDevice() const noexcept;
    VkShaderModule GetHandle() const noexcept;
    explicit(false) operator VkShaderModule() const noexcept;
    explicit(false) operator bool() const noexcept;

  private:
    LogicalDevice::Proxy m_Device{};
    VkShaderModule m_Module = VK_NULL_HANDLE;
};
} // namespace VKit
