#pragma once

#include "vkit/vulkan/vulkan.hpp"
#include "tkit/container/static_array.hpp"
#include <functional>

namespace VKit
{
/**
 * @brief Provides system-wide utilities for querying and managing Vulkan layers and extensions.
 *
 * Includes methods to check for support, retrieve details about layers and extensions,
 * and fetch Vulkan functions at the instance or device level.
 */
struct VKIT_API System
{
    /**
     * @brief Initializes the Vulkan system.
     *
     * Prepares the system by loading available extensions and layers.
     * This should be called before any other Vulkit operations.
     *
     * @return A `Result` indicating success or an error if initialization fails.
     */
    static Result<> Initialize() noexcept;
    static void Terminate() noexcept;

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

    static inline TKit::StaticArray64<VkExtensionProperties> AvailableExtensions{};
    static inline TKit::StaticArray16<VkLayerProperties> AvailableLayers{};
};

/**
 * @brief Manages deferred deletion of Vulkan resources.
 *
 * Allows users to enqueue resource cleanup operations, which can be flushed
 * in bulk to ensure proper resource management.
 */
class VKIT_API DeletionQueue
{
  public:
    void Push(std::function<void()> &&p_Deleter) noexcept;
    void Flush() noexcept;

    template <typename VKitObject> void SubmitForDeletion(const VKitObject &p_Object) noexcept
    {
        p_Object.SubmitForDeletion(*this);
    }

  private:
    TKit::StaticArray1024<std::function<void()>> m_Deleters;
};

VKIT_API const char *VkResultToString(VkResult p_Result) noexcept;

} // namespace VKit