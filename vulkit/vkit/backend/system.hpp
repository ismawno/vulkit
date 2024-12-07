#pragma once

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

#define VKIT_FORMAT_ERROR(p_Result, ...) VulkanResultInfo<std::string>::Error(p_Result, TKIT_FORMAT(__VA_ARGS__))

namespace VKit
{
template <typename T>
concept String = std::is_same_v<T, const char *> || std::is_same_v<T, std::string>;

template <String MessageType> class VulkanResultInfo
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

struct System
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

class DeletionQueue
{
  public:
    void Push(std::function<void()> &&p_Deleter) noexcept;
    void Flush() noexcept;

  private:
    DynamicArray<std::function<void()>> m_Deleters;
};

} // namespace VKit