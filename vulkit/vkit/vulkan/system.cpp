#include "vkit/core/pch.hpp"
#include "vkit/vulkan/system.hpp"

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

Result<> System::Initialize() noexcept
{
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

void System::Terminate() noexcept
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

const VkExtensionProperties *System::GetExtension(const char *p_Name) noexcept
{
    for (const VkExtensionProperties &extension : AvailableExtensions)
        if (strcmp(p_Name, extension.extensionName) == 0)
            return &extension;
    return nullptr;
}

const VkLayerProperties *System::GetLayer(const char *p_Name) noexcept
{
    for (const VkLayerProperties &layer : AvailableLayers)
        if (strcmp(p_Name, layer.layerName) == 0)
            return &layer;
    return nullptr;
}

bool System::IsExtensionSupported(const char *p_Name) noexcept
{
    for (const VkExtensionProperties &extension : AvailableExtensions)
        if (strcmp(p_Name, extension.extensionName) == 0)
            return true;
    return false;
}

bool System::IsLayerSupported(const char *p_Name) noexcept
{
    for (const VkLayerProperties &layer : AvailableLayers)
        if (strcmp(p_Name, layer.layerName) == 0)
            return true;
    return false;
}

void DeletionQueue::Push(std::function<void()> &&p_Deleter) noexcept
{
    m_Deleters.Append(std::move(p_Deleter));
}
void DeletionQueue::Flush() noexcept
{
    for (u32 i = m_Deleters.GetSize(); i > 0; --i)
        m_Deleters[i - 1]();
    m_Deleters.Clear();
}

const char *VkResultToString(const VkResult p_Result) noexcept
{
    switch (p_Result)
    {
    case VK_SUCCESS:
        return "VK_SUCCESS";
    case VK_NOT_READY:
        return "VK_NOT_READY";
    case VK_TIMEOUT:
        return "VK_TIMEOUT";
    case VK_EVENT_SET:
        return "VK_EVENT_SET";
    case VK_EVENT_RESET:
        return "VK_EVENT_RESET";
    case VK_INCOMPLETE:
        return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
        return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
        return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:
        return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
        return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
        return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS:
        return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
        return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL:
        return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_UNKNOWN:
        return "VK_ERROR_UNKNOWN";
    case VK_ERROR_OUT_OF_POOL_MEMORY:
        return "VK_ERROR_OUT_OF_POOL_MEMORY";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
        return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case VK_ERROR_FRAGMENTATION:
        return "VK_ERROR_FRAGMENTATION";
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
        return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
    case VK_PIPELINE_COMPILE_REQUIRED:
        return "VK_PIPELINE_COMPILE_REQUIRED";
    case VK_ERROR_SURFACE_LOST_KHR:
        return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR:
        return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR:
        return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT:
        return "VK_ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_INVALID_SHADER_NV:
        return "VK_ERROR_INVALID_SHADER_NV";
    case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
        return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
        return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
        return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
        return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
        return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
        return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
        return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
    case VK_ERROR_NOT_PERMITTED_KHR:
        return "VK_ERROR_NOT_PERMITTED_KHR";
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
        return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
    case VK_THREAD_IDLE_KHR:
        return "VK_THREAD_IDLE_KHR";
    case VK_THREAD_DONE_KHR:
        return "VK_THREAD_DONE_KHR";
    case VK_OPERATION_DEFERRED_KHR:
        return "VK_OPERATION_DEFERRED_KHR";
    case VK_OPERATION_NOT_DEFERRED_KHR:
        return "VK_OPERATION_NOT_DEFERRED_KHR";
    case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
        return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
    case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT:
        return "VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT";
#ifdef VK_ENABLE_BETA_EXTENSIONS
    case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
        return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
#endif
    default:
        return "Unknown VkResult";
    }
}

} // namespace VKit
