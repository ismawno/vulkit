#pragma once

#include "vkit/core/alias.hpp"
#include "tkit/utilities/result.hpp"
#include <vulkan/vulkan.hpp>
#include <span>

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

#define VKIT_ERROR(p_Result, ...) VulkanResult::Error(p_Result, TKIT_FORMAT(__VA_ARGS__))

namespace VKit
{
class VulkanResult
{
  public:
    static VulkanResult Success() noexcept;
    static VulkanResult Error(VkResult p_Result, std::string_view p_Message) noexcept;

    explicit(false) operator bool() const noexcept;

    VkResult Result = VK_SUCCESS;
    std::string Message;

  private:
    VulkanResult() noexcept = default;
    VulkanResult(VkResult p_Result, std::string_view p_Message) noexcept;
};

template <typename T> using Result = TKit::Result<T, VulkanResult>;

struct System
{
    static VulkanResult Initialize() noexcept;

    static bool IsExtensionSupported(const char *p_Name) noexcept;
    static bool IsLayerSupported(const char *p_Name) noexcept;

    static bool AreExtensionsSupported(std::span<const char *const> p_Names) noexcept;
    static bool AreLayersSupported(std::span<const char *const> p_Names) noexcept;

    static const VkExtensionProperties *GetExtension(const char *p_Name) noexcept;
    static const VkLayerProperties *GetLayer(const char *p_Name) noexcept;

    template <typename F>
    static F GetVulkanFunction(const char *p_Name, const VkInstance p_Instance = VK_NULL_HANDLE) noexcept
    {
        return reinterpret_cast<F>(vkGetInstanceProcAddr(p_Instance, p_Name));
    }

    static DynamicArray<VkExtensionProperties> Extensions;
    static DynamicArray<VkLayerProperties> Layers;
};
} // namespace VKit