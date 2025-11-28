#include "vkit/core/pch.hpp"
#include "vkit/core/core.hpp"
#include "vkit/vulkan/loader.hpp"
#include "vkit/core/alias.hpp"

namespace VKit
{

const VkExtensionProperties *Core::GetExtension(const char *p_Name)
{
    for (const VkExtensionProperties &extension : AvailableExtensions)
        if (strcmp(p_Name, extension.extensionName) == 0)
            return &extension;
    return nullptr;
}

const VkLayerProperties *Core::GetLayer(const char *p_Name)
{
    for (const VkLayerProperties &layer : AvailableLayers)
        if (strcmp(p_Name, layer.layerName) == 0)
            return &layer;
    return nullptr;
}

bool Core::IsExtensionSupported(const char *p_Name)
{
    for (const VkExtensionProperties &extension : AvailableExtensions)
        if (strcmp(p_Name, extension.extensionName) == 0)
            return true;
    return false;
}

bool Core::IsLayerSupported(const char *p_Name)
{
    for (const VkLayerProperties &layer : AvailableLayers)
        if (strcmp(p_Name, layer.layerName) == 0)
            return true;
    return false;
}
} // namespace VKit

#if defined(TKIT_OS_APPLE) || defined(TKIT_OS_LINUX)
#    include <dlfcn.h>
#elif defined(TKIT_OS_WINDOWS)
#    include "tkit/core/windows.hpp"
#else
#    error "[VULKIT] Unsupported platform to load Vulkan library"
#endif

namespace VKit
{
static void *s_Library = nullptr;
static void attempt(const char *p_LoaderPath)
{
    if (s_Library)
        return;

    TKIT_LOG_INFO("[VULKIT] Attempting to load vulkan library. Trying: {}", p_LoaderPath);
#ifdef TKIT_OS_WINDOWS
    s_Library = reinterpret_cast<void *>(LoadLibraryA(p_LoaderPath));
#else
    s_Library = dlopen(p_LoaderPath, RTLD_NOW | RTLD_LOCAL);
#endif

    TKIT_LOG_INFO_IF(s_Library, "[VULKIT] Success");
    TKIT_LOG_WARNING_IF(!s_Library, "[VULKIT] Failed");
}

Result<> Core::Initialize(const char *p_LoaderPath)
{
    if (s_Library)
        return Result<>::Ok();

    if (p_LoaderPath)
        attempt(p_LoaderPath);
#if defined(TKIT_OS_APPLE)
    attempt("libvulkan.dylib");
    attempt("libvulkan.1.dylib");
    attempt("libMoltenVK.dylib");
    attempt("@executable_path/../Frameworks/libvulkan.dylib");
    attempt("@executable_path/../Frameworks/libvulkan.1.dylib");
    attempt("@executable_path/../Frameworks/libMoltenVK.dylib");
#elif defined(TKIT_OS_LINUX)
    attempt("libvulkan.so");
    attempt("libvulkan.so.1");
#else
    attempt("vulkan-1.dll");
    attempt("vulkan.dll");
#endif
    if (!s_Library)
        return Result<>::Error(VK_ERROR_INITIALIZATION_FAILED,
                               "Failed to load Vulkan library. All attempts have been exhausted. You may try "
                               "specifying a custom path for the vulkan library");

    Vulkan::Load(s_Library);

    VKIT_CHECK_GLOBAL_FUNCTION_OR_RETURN(vkEnumerateInstanceExtensionProperties, Result<>);
    VKIT_CHECK_GLOBAL_FUNCTION_OR_RETURN(vkEnumerateInstanceLayerProperties, Result<>);

    u32 extensionCount = 0;
    VkResult result;
    result = Vulkan::EnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to get the number of instance extensions");

    AvailableExtensions.Resize(extensionCount);
    result = Vulkan::EnumerateInstanceExtensionProperties(nullptr, &extensionCount, AvailableExtensions.GetData());
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to get the instance extensions");

    u32 layerCount = 0;
    result = Vulkan::EnumerateInstanceLayerProperties(&layerCount, nullptr);
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to get the number of instance layers");

    AvailableLayers.Resize(layerCount);
    result = Vulkan::EnumerateInstanceLayerProperties(&layerCount, AvailableLayers.GetData());
    if (result != VK_SUCCESS)
        return Result<>::Error(result, "Failed to get the instance layers");

    return Result<>::Ok();
}
void Core::Terminate()
{
    if (!s_Library)
        return;
#if defined(TKIT_OS_APPLE) || defined(TKIT_OS_LINUX)
    dlclose(s_Library);
#else
    FreeLibrary(reinterpret_cast<HMODULE>(s_Library));
#endif
    s_Library = nullptr;
}
} // namespace VKit
