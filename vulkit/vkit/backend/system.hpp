#pragma once

#include "vkit/core/api.hpp"
#include "vkit/core/alias.hpp"
#include "tkit/utilities/result.hpp"
#include <vulkan/vulkan.hpp>
#include <span>
#include <functional>

#ifdef VK_MAKE_API_VERSION
#    define VKIT_MAKE_VERSION(variant, major, minor, patch) VK_MAKE_API_VERSION(variant, major, minor, patch)
#elif defined(VK_MAKE_VERSION)
#    define VKIT_MAKE_VERSION(variant, major, minor, patch) VK_MAKE_VERSION(major, minor, patch)
#endif

#if defined(VK_API_VERSION_1_3) || defined(VK_VERSION_1_3)
#    define VKIT_API_VERSION_1_3 VKIT_MAKE_VERSION(0, 1, 3, 0)
#endif

#if defined(VK_API_VERSION_1_2) || defined(VK_VERSION_1_2)
#    define VKIT_API_VERSION_1_2 VKIT_MAKE_VERSION(0, 1, 2, 0)
#endif

#if defined(VK_API_VERSION_1_1) || defined(VK_VERSION_1_1)
#    define VKIT_API_VERSION_1_1 VKIT_MAKE_VERSION(0, 1, 1, 0)
#endif

#if defined(VK_API_VERSION_1_0) || defined(VK_VERSION_1_0)
#    define VKIT_API_VERSION_1_0 VKIT_MAKE_VERSION(0, 1, 0, 0)
#endif

#ifdef TKIT_ENABLE_ASSERTS
#    define VKIT_ASSERT_VULKAN_RESULT(result) TKIT_ASSERT(result, "{}", result.Message)
#    define VKIT_ASSERT_RESULT(result) VKIT_ASSERT_VULKAN_RESULT(result.GetError())
#else
#    define VKIT_ASSERT_VULKAN_RESULT(result)
#    define VKIT_ASSERT_RESULT(result)
#endif

#ifdef TKIT_ENABLE_INFO_LOGS
#    define VKIT_LOG_VULKAN_RESULT(result) TKIT_LOG_INFO("{}", result.Message)
#    define VKIT_LOG_RESULT(result) VKIT_LOG_VULKAN_RESULT(result.GetError())
#else
#    define VKIT_LOG_VULKAN_RESULT(result)
#    define VKIT_LOG_RESULT(result)
#endif

#ifdef TKIT_ENABLE_WARNING_LOGS
#    define VKIT_WARN_VULKAN_RESULT(result) TKIT_LOG_WARNING("{}", result.Message)
#    define VKIT_WARN_RESULT(result) VKIT_WARN_VULKAN_RESULT(result.GetError())
#else
#    define VKIT_WARN_VULKAN_RESULT(result)
#    define VKIT_WARN_RESULT(result)
#endif

#define VKIT_FORMAT_ERROR(p_Result, ...) VulkanResultInfo<std::string>::Error(p_Result, TKIT_FORMAT(__VA_ARGS__))

namespace VKit
{
template <typename T>
concept String = std::is_same_v<T, const char *> || std::is_same_v<T, std::string>;

template <String MessageType> class VKIT_API VulkanResultInfo
{
  public:
    static VulkanResultInfo Success() noexcept;
    static VulkanResultInfo Error(VkResult p_Result, const MessageType &p_Message) noexcept;

    VulkanResultInfo() noexcept = default;
    VulkanResultInfo(VkResult p_Result, const MessageType &p_Message) noexcept;

    explicit(false) operator bool() const noexcept;

    VkResult Result = VK_SUCCESS;
    MessageType Message{};
};

using VulkanResult = VulkanResultInfo<const char *>;
using VulkanFormattedResult = VulkanResultInfo<std::string>;

template <typename T> using Result = TKit::Result<T, VulkanResult>;
template <typename T> using FormattedResult = TKit::Result<T, VulkanFormattedResult>;

struct VKIT_API System
{
    static VulkanResult Initialize() noexcept;

    static bool IsExtensionSupported(const char *p_Name) noexcept;
    static bool IsLayerSupported(const char *p_Name) noexcept;

    static const VkExtensionProperties *GetExtension(const char *p_Name) noexcept;
    static const VkLayerProperties *GetLayer(const char *p_Name) noexcept;

    template <typename F>
    static F GetInstanceFunction(const char *p_Name, const VkInstance p_Instance = VK_NULL_HANDLE) noexcept
    {
        return reinterpret_cast<F>(vkGetInstanceProcAddr(p_Instance, p_Name));
    }
    template <typename F> static F GetDeviceFunction(const char *p_Name, const VkDevice p_Device) noexcept
    {
        return reinterpret_cast<F>(vkGetDeviceProcAddr(p_Device, p_Name));
    }

    static inline DynamicArray<VkExtensionProperties> AvailableExtensions{};
    static inline DynamicArray<VkLayerProperties> AvailableLayers{};
};

class VKIT_API DeletionQueue
{
  public:
    void Push(std::function<void()> &&p_Deleter) noexcept;
    void Flush() noexcept;

  private:
    DynamicArray<std::function<void()>> m_Deleters;
};

} // namespace VKit