#pragma once

#include "vkit/core/api.hpp"
#include "vkit/core/alias.hpp"
#include "vkit/backend/system.hpp"
#include "tkit/memory/ptr.hpp"
#include "tkit/utilities/result.hpp"
#include <vulkan/vulkan.h>

namespace VKit
{
/**
 * @brief Represents a Vulkan instance.
 *
 * A handle to the Vulkan API that manages extensions, layers, and debug configurations.
 * The underlying resources must be destroyed explicitly using the `Destroy` method.
 */
class VKIT_API Instance
{
  public:
    /**
     * @brief A utility for setting up and creating a Vulkan instance.
     *
     * Provides methods to define application details, API versions, extensions,
     * and layers. `Require` methods enforce strict conditions, while `Request`
     * methods try to enable features without failing if unavailable.
     */
    class Builder
    {
      public:
        /**
         * @brief Creates a Vulkan instance with the specified configuration.
         *
         * Returns a valid instance if all required parameters are met, or an error otherwise.
         *
         * @return A result containing the created Instance or an error.
         */
        FormattedResult<Instance> Build() const noexcept;

        Builder &SetApplicationName(const char *p_Name) noexcept;
        Builder &SetEngineName(const char *p_Name) noexcept;

        Builder &SetApplicationVersion(u32 p_Version) noexcept;
        Builder &SetEngineVersion(u32 p_Version) noexcept;

        Builder &SetApplicationVersion(u32 p_Major, u32 p_Minor, u32 p_Patch) noexcept;
        Builder &SetEngineVersion(u32 p_Major, u32 p_Minor, u32 p_Patch) noexcept;

        Builder &RequireApiVersion(u32 p_Version) noexcept;
        Builder &RequireApiVersion(u32 p_Major, u32 p_Minor, u32 p_Patch) noexcept;

        Builder &RequestApiVersion(u32 p_Version) noexcept;
        Builder &RequestApiVersion(u32 p_Major, u32 p_Minor, u32 p_Patch) noexcept;

        Builder &RequireExtension(const char *p_Extension) noexcept;
        Builder &RequireExtensions(std::span<const char *const> p_Extensions) noexcept;

        Builder &RequestExtension(const char *p_Extension) noexcept;
        Builder &RequestExtensions(std::span<const char *const> p_Extensions) noexcept;

        Builder &RequireLayer(const char *p_Layer) noexcept;
        Builder &RequireLayers(std::span<const char *const> p_Layers) noexcept;

        Builder &RequestLayer(const char *p_Layer) noexcept;
        Builder &RequestLayers(std::span<const char *const> p_Layers) noexcept;

        Builder &RequireValidationLayers() noexcept;
        Builder &RequestValidationLayers() noexcept;

        Builder &SetDebugCallback(PFN_vkDebugUtilsMessengerCallbackEXT p_Callback) noexcept;
        Builder &SetHeadless(bool p_Headless = true) noexcept;

        Builder &SetDebugMessengerUserData(void *p_Data) noexcept;
        Builder &SetAllocationCallbacks(const VkAllocationCallbacks *p_AllocationCallbacks) noexcept;

      private:
        const char *m_ApplicationName = nullptr;
        const char *m_EngineName = nullptr;

        u32 m_ApplicationVersion = VKIT_MAKE_VERSION(0, 1, 0, 0);
        u32 m_EngineVersion = VKIT_MAKE_VERSION(0, 1, 0, 0);
        u32 m_RequiredApiVersion = VKIT_MAKE_VERSION(0, 1, 0, 0);
        u32 m_RequestedApiVersion = VKIT_MAKE_VERSION(0, 1, 0, 0);

        TKit::StaticArray64<const char *> m_RequiredExtensions;
        TKit::StaticArray64<const char *> m_RequestedExtensions;

        TKit::StaticArray16<const char *> m_RequiredLayers;
        TKit::StaticArray16<const char *> m_RequestedLayers;

        bool m_RequireValidationLayers = false;
        bool m_RequestValidationLayers = false;
        bool m_Headless = false;

        void *m_DebugMessengerUserData = nullptr;
        const VkAllocationCallbacks *m_AllocationCallbacks = nullptr;

        PFN_vkDebugUtilsMessengerCallbackEXT m_DebugCallback = nullptr;
    };

    /**
     * @brief Flags for configuring instance behavior.
     *
     * Use these flags to enable features like headless mode, validation layers,
     * or specific Vulkan extensions during instance creation.
     */
    enum FlagBits : u8
    {
        Flag_Headless = 1 << 0,
        Flag_HasValidationLayers = 1 << 1,
        Flag_Properties2Extension = 1 << 2
    };
    using Flags = u8;

    /**
     * @brief Stores the configuration details for a Vulkan instance.
     *
     * Includes the application and engine names, API version, enabled extensions,
     * and layers. It also contains optional settings for debugging and memory allocation.
     */
    struct Info
    {
        const char *ApplicationName;
        const char *EngineName;

        u32 ApplicationVersion;
        u32 EngineVersion;
        u32 ApiVersion;

        Flags Flags;

        TKit::StaticArray64<const char *> EnabledExtensions;
        TKit::StaticArray16<const char *> EnabledLayers;

        VkDebugUtilsMessengerEXT DebugMessenger;
        const VkAllocationCallbacks *AllocationCallbacks;
    };

    Instance() noexcept = default;
    Instance(VkInstance p_Instance, const Info &p_Info) noexcept;

    bool IsExtensionEnabled(const char *p_Extension) const noexcept;
    bool IsLayerEnabled(const char *p_Layer) const noexcept;

    void Destroy() noexcept;
    void SubmitForDeletion(DeletionQueue &p_Queue) const noexcept;

    VkInstance GetInstance() const noexcept;
    const Info &GetInfo() const noexcept;

    explicit(false) operator VkInstance() const noexcept;
    explicit(false) operator bool() const noexcept;

    /**
     * @brief Retrieves an instance function by name.
     *
     * @tparam F Function type to cast to.
     * @param p_Name Name of the function.
     * @return The function pointer casted to the specified type.
     */
    template <typename F> F GetFunction(const char *p_Name) const noexcept
    {
        return reinterpret_cast<F>(vkGetInstanceProcAddr(m_Instance, p_Name));
    }

  private:
    VkInstance m_Instance = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit
