#include "vkit/core/pch.hpp"
#include "tkit/utils/logging.hpp"
#include "vkit/vulkan/vulkan.hpp"
#include "vkit/core/alias.hpp"

namespace VKit
{
using namespace Detail;

template <typename T> static std::string formatMessage(const VkResult p_Result, const T &p_Message)
{
    return TKit::Format("[VULKIT] VkResult: '{}' - Message: '{}'", VulkanResultToString(p_Result), p_Message);
}

std::string Error::ToString() const
{
    std::string str = TKit::Format("[VULKIT] Error code: '{}'", ErrorCodeToString(m_ErrorCode));
    if (m_ErrorCode == Error_VulkanError)
        str += TKit::Format(" - Vulkan result: '{}'", VulkanResultToString(m_VkResult));
    if (m_CheapMessage)
        str += TKit::Format(" - Message: '{}'", m_CheapMessage);
    else if (!m_FormattedMessage.empty())
        str += TKit::Format(" - Message: '{}'", m_FormattedMessage);
    return str;
}

void DeletionQueue::Push(std::function<void()> &&p_Deleter)
{
    m_Deleters.Append(std::move(p_Deleter));
}
void DeletionQueue::Flush()
{
    for (u32 i = m_Deleters.GetSize(); i > 0; --i)
        m_Deleters[i - 1]();
    m_Deleters.Clear();
}

const char *ErrorCodeToString(const ErrorCode p_Code)
{
    switch (p_Code)
    {
    case Error_VulkanError:
        return "Vulkan error";
    case Error_VulkanLibraryNotFound:
        return "Vulkan library not found";
    case Error_VulkanFunctionNotLoaded:
        return "Vulkan function not loaded";
    case Error_BadInput:
        return "Bad input";
    case Error_VersionMismatch:
        return "Version mismatch";
    case Error_NoSurfaceCapabilities:
        return "No surface capabilities";
    case Error_RejectedDevice:
        return "Rejected device";
    case Error_MissingQueue:
        return "Missing queue";
    case Error_MissingExtension:
        return "Missing extension";
    case Error_MissingFeature:
        return "Missing feature";
    case Error_MissingLayer:
        return "Missing layer";
    case Error_InsufficientMemory:
        return "Insufficient memory";
    case Error_NoDeviceFound:
        return "No device found";
    case Error_NoFormatSupported:
        return "No supported format";
    case Error_BadImageCount:
        return "Bad image count";
    case Error_FileNotFound:
        return "File not found";
    case Error_Unknown:
        return "Unknown error";
    }
    return "Unknown error";
}

const char *VulkanResultToString(const VkResult p_Result)
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

#if defined(VKIT_API_VERSION_1_2)
    case VK_ERROR_UNKNOWN:
        return "VK_ERROR_UNKNOWN";
    case VK_ERROR_FRAGMENTATION:
        return "VK_ERROR_FRAGMENTATION";
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
        return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
#endif

#if defined(VKIT_API_VERSION_1_3)
    case VK_PIPELINE_COMPILE_REQUIRED:
        return "VK_PIPELINE_COMPILE_REQUIRED";
#endif

#if defined(VKIT_API_VERSION_1_4)
    case VK_ERROR_NOT_PERMITTED:
        return "VK_ERROR_NOT_PERMITTED";
#endif

    case VK_ERROR_OUT_OF_POOL_MEMORY:
        return "VK_ERROR_OUT_OF_POOL_MEMORY";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
        return "VK_ERROR_INVALID_EXTERNAL_HANDLE";

#if defined(VK_KHR_surface)
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
#endif

#if defined(VK_EXT_validation_features) || defined(VK_EXT_debug_report)
    case VK_ERROR_VALIDATION_FAILED_EXT:
        return "VK_ERROR_VALIDATION_FAILED_EXT";
#endif

#ifdef VK_NV_glsl_shader
    case VK_ERROR_INVALID_SHADER_NV:
        return "VK_ERROR_INVALID_SHADER_NV";
#endif

#ifdef VK_EXT_image_drm_format_modifier
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
        return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
#endif

#ifdef VK_EXT_full_screen_exclusive
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
        return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
#endif

#ifdef VK_KHR_deferred_host_operations
    case VK_THREAD_IDLE_KHR:
        return "VK_THREAD_IDLE_KHR";
    case VK_THREAD_DONE_KHR:
        return "VK_THREAD_DONE_KHR";
    case VK_OPERATION_DEFERRED_KHR:
        return "VK_OPERATION_DEFERRED_KHR";
    case VK_OPERATION_NOT_DEFERRED_KHR:
        return "VK_OPERATION_NOT_DEFERRED_KHR";
#endif

#ifdef VK_KHR_video_queue
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
    case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
        return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
#endif

#ifdef VK_EXT_shader_object
    case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT:
        return "VK_INCOMPATIBLE_SHADER_BINARY_EXT";
#endif

#ifdef VK_KHR_pipeline_binary
    case VK_PIPELINE_BINARY_MISSING_KHR:
        return "VK_PIPELINE_BINARY_MISSING_KHR";
    case VK_ERROR_NOT_ENOUGH_SPACE_KHR:
        return "VK_ERROR_NOT_ENOUGH_SPACE_KHR";
#endif

#ifdef VK_EXT_image_compression_control
    case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
        return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
#endif

    default:
        return "Unknown VkResult";
    }
}
} // namespace VKit
