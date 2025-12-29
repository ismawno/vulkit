#pragma once

#include "vkit/vulkan/vulkan.hpp"
#include "tkit/container/array.hpp"

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
    static Result<> Initialize(const char *p_LoaderPath = nullptr);
    static void Terminate();

    static bool IsExtensionSupported(const char *p_Name);
    static bool IsLayerSupported(const char *p_Name);

    static const VkExtensionProperties *GetExtension(const char *p_Name);
    static const VkLayerProperties *GetLayer(const char *p_Name);

    static inline TKit::Array64<VkExtensionProperties> AvailableExtensions{};
    static inline TKit::Array16<VkLayerProperties> AvailableLayers{};
};

} // namespace VKit
