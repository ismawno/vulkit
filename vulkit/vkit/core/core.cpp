#include "vkit/core/pch.hpp"
#include "vkit/core/core.hpp"
#include "vkit/vulkan/loader.hpp"
#include "vkit/core/alias.hpp"

#if defined(TKIT_OS_APPLE) || defined(TKIT_OS_LINUX)
#    include <dlfcn.h>
#elif defined(TKIT_OS_WINDOWS)
#    include <windows.h>
#else
#    error "[VULKIT] Unsupported platform to load Vulkan library"
#endif

namespace VKit
{
#if defined(TKIT_OS_APPLE) || defined(TKIT_OS_LINUX)
static void *s_Library = nullptr;
#else
static HMODULE s_Library = nullptr;
#endif

Result<> Core::Initialize()
{
    if (s_Library)
        return Result<>::Ok();

#if defined(TKIT_OS_APPLE)
    s_Library = dlopen("libvulkan.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!s_Library)
        s_Library = dlopen("libvulkan.1.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!s_Library)
        s_Library = dlopen("libMoltenVK.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!s_Library)
        s_Library = dlopen("@executable_path/../Frameworks/libvulkan.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!s_Library)
        s_Library = dlopen("@executable_path/../Frameworks/libvulkan.1.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!s_Library)
        s_Library = dlopen("@executable_path/../Frameworks/libMoltenVK.dylib", RTLD_NOW | RTLD_LOCAL);
#elif defined(TKIT_OS_LINUX)
    s_Library = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (!s_Library)
        s_Library = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
#else
    s_Library = LoadLibraryA("vulkan-1.dll");
    if (!s_Library)
        s_Library = LoadLibraryA("vulkan.dll");
#endif
    if (!s_Library)
        return Result<>::Error(VK_ERROR_INITIALIZATION_FAILED, "Failed to load Vulkan library");

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
    FreeLibrary(s_Library);
#endif
    s_Library = nullptr;
}

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
