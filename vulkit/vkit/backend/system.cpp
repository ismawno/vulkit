#include "vkit/core/pch.hpp"
#include "vkit/backend/system.hpp"

namespace VKit
{
template <String MessageType>
VulkanResultInfo<MessageType>::VulkanResultInfo(VkResult p_Result, const MessageType &p_Message) noexcept
    : Result(p_Result), Message(p_Message)
{
}

template <String MessageType> VulkanResultInfo<MessageType> VulkanResultInfo<MessageType>::Success() noexcept
{
    return VulkanResultInfo{};
}
template <String MessageType>
VulkanResultInfo<MessageType> VulkanResultInfo<MessageType>::Error(VkResult p_Result,
                                                                   const MessageType &p_Message) noexcept
{
    return VulkanResultInfo(p_Result, p_Message);
}

template <String MessageType> VulkanResultInfo<MessageType>::operator bool() const noexcept
{
    return Result == VK_SUCCESS;
}

template class VKIT_API VulkanResultInfo<const char *>;
template class VKIT_API VulkanResultInfo<std::string>;

VulkanFormattedResult ToFormatted(const VulkanResult &p_Result) noexcept
{
    return VulkanFormattedResult{p_Result.Result, p_Result.Message};
}

VulkanResult System::Initialize() noexcept
{
    const auto enumerateExtensions =
        GetInstanceFunction<PFN_vkEnumerateInstanceExtensionProperties>("vkEnumerateInstanceExtensionProperties");
    if (!enumerateExtensions)
        return VulkanResult::Error(VK_ERROR_EXTENSION_NOT_PRESENT,
                                   "Failed to get the vkEnumerateInstanceExtensionProperties function");

    u32 extensionCount = 0;
    VkResult result;
    result = enumerateExtensions(nullptr, &extensionCount, nullptr);
    if (result != VK_SUCCESS)
        return VulkanResult::Error(result, "Failed to get the number of instance extensions");

    AvailableExtensions.resize(extensionCount);
    result = enumerateExtensions(nullptr, &extensionCount, AvailableExtensions.data());
    if (result != VK_SUCCESS)
        return VulkanResult::Error(result, "Failed to get the instance extensions");

    const auto enumerateLayers =
        GetInstanceFunction<PFN_vkEnumerateInstanceLayerProperties>("vkEnumerateInstanceLayerProperties");
    if (!enumerateLayers)
        return VulkanResult::Error(VK_ERROR_EXTENSION_NOT_PRESENT,
                                   "Failed to get the vkEnumerateInstanceLayerProperties function");

    u32 layerCount = 0;
    result = enumerateLayers(&layerCount, nullptr);
    if (result != VK_SUCCESS)
        return VulkanResult::Error(result, "Failed to get the number of instance layers");

    AvailableLayers.resize(layerCount);
    result = enumerateLayers(&layerCount, AvailableLayers.data());
    if (result != VK_SUCCESS)
        return VulkanResult::Error(result, "Failed to get the instance layers");

    return VulkanResult::Success();
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
    m_Deleters.push_back(std::move(p_Deleter));
}
void DeletionQueue::Flush() noexcept
{
    for (auto it = m_Deleters.rbegin(); it != m_Deleters.rend(); ++it)
        (*it)();
    m_Deleters.clear();
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