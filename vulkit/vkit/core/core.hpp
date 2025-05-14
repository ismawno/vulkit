#pragma once

#include "vkit/vulkan/vulkan.hpp"
#include "vkit/vulkan/loader.hpp"
#include "tkit/container/static_array.hpp"
#include <functional>

namespace VKit
{
/**
 * @brief Provides system-wide utilities for querying and managing Vulkan layers and extensions.
 *
 * Includes methods to check for support, retrieve details about layers and extensions,
 * and fetch Vulkan functions at the instance or device level.
 */
struct VKIT_API Core
{
    /**
     * @brief Initializes the Vulkan system.
     *
     * Prepares the system by loading available extensions and layers.
     * This should be called before any other Vulkit operations.
     *
     * @return A `Result` indicating success or an error if initialization fails.
     */
    static Result<> Initialize() noexcept;
    static void Terminate() noexcept;

    static bool IsExtensionSupported(const char *p_Name) noexcept;
    static bool IsLayerSupported(const char *p_Name) noexcept;

    static const VkExtensionProperties *GetExtension(const char *p_Name) noexcept;
    static const VkLayerProperties *GetLayer(const char *p_Name) noexcept;

    static inline TKit::StaticArray64<VkExtensionProperties> AvailableExtensions{};
    static inline TKit::StaticArray16<VkLayerProperties> AvailableLayers{};
};

} // namespace VKit