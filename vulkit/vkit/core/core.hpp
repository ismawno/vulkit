#pragma once

#include "vkit/vulkan/vulkan.hpp"
#include "tkit/container/array.hpp"

namespace VKit
{
struct Core
{
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
