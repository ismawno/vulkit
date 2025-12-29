#pragma once

#ifndef VKIT_ENABLE_INSTANCE
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_INSTANCE"
#endif

#include "vkit/core/api.hpp"
#include "vkit/vulkan/loader.hpp"
#include "vkit/core/alias.hpp"
#include "tkit/container/span.hpp"
#include <vulkan/vulkan.h>

namespace VKit
{
/**
 * @brief Represents a Vulkan instance.
 *
 * A handle to the Vulkan API that manages extensions, layers, and debug configurations.
 * The underlying resources must be destroyed explicitly using the `Destroy()` method.
 */
class VKIT_API Instance
{
  public:
    struct Proxy
    {
        VkInstance Instance = VK_NULL_HANDLE;
        const VkAllocationCallbacks *AllocationCallbacks = nullptr;
        const Vulkan::InstanceTable *Table = nullptr;

        operator bool() const
        {
            return Instance != VK_NULL_HANDLE;
        }
        operator VkInstance() const
        {
            return Instance;
        }
    };

    /**
     * @brief A utility for setting up and creating a Vulkan instance.
     *
     * Provides methods to define application details, API versions, extensions,
     * and layers. `Require()` methods enforce strict conditions, while `Request()`
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
         * @return A `Result` containing the created `Instance` or an error.
         */
        Result<Instance> Build() const;

        Builder &SetApplicationName(const char *p_Name);
        Builder &SetEngineName(const char *p_Name);

        Builder &SetApplicationVersion(u32 p_Version);
        Builder &SetEngineVersion(u32 p_Version);

        Builder &SetApplicationVersion(u32 p_Major, u32 p_Minor, u32 p_Patch);
        Builder &SetEngineVersion(u32 p_Major, u32 p_Minor, u32 p_Patch);

        Builder &RequireApiVersion(u32 p_Version);
        Builder &RequireApiVersion(u32 p_Major, u32 p_Minor, u32 p_Patch);

        Builder &RequestApiVersion(u32 p_Version);
        Builder &RequestApiVersion(u32 p_Major, u32 p_Minor, u32 p_Patch);

        Builder &RequireExtension(const char *p_Extension);
        Builder &RequireExtensions(TKit::Span<const char *const> p_Extensions);

        Builder &RequestExtension(const char *p_Extension);
        Builder &RequestExtensions(TKit::Span<const char *const> p_Extensions);

        Builder &RequireLayer(const char *p_Layer);
        Builder &RequireLayers(TKit::Span<const char *const> p_Layers);

        Builder &RequestLayer(const char *p_Layer);
        Builder &RequestLayers(TKit::Span<const char *const> p_Layers);

        Builder &RequireValidationLayers();
        Builder &RequestValidationLayers();

        Builder &SetDebugCallback(PFN_vkDebugUtilsMessengerCallbackEXT p_Callback);
        Builder &SetHeadless(bool p_Headless = true);

        Builder &SetDebugMessengerUserData(void *p_Data);
        Builder &SetAllocationCallbacks(const VkAllocationCallbacks *p_AllocationCallbacks);

      private:
        const char *m_ApplicationName = nullptr;
        const char *m_EngineName = nullptr;

        u32 m_ApplicationVersion = VKIT_MAKE_VERSION(0, 1, 0, 0);
        u32 m_EngineVersion = VKIT_MAKE_VERSION(0, 1, 0, 0);
        u32 m_RequiredApiVersion = VKIT_MAKE_VERSION(0, 1, 0, 0);
        u32 m_RequestedApiVersion = VKIT_MAKE_VERSION(0, 1, 0, 0);

        TKit::Array64<const char *> m_RequiredExtensions;
        TKit::Array64<const char *> m_RequestedExtensions;

        TKit::Array16<const char *> m_RequiredLayers;
        TKit::Array16<const char *> m_RequestedLayers;

        bool m_RequireValidationLayers = false;
        bool m_RequestValidationLayers = false;
        bool m_Headless = false;

        void *m_DebugMessengerUserData = nullptr;
        const VkAllocationCallbacks *m_AllocationCallbacks = nullptr;

        PFN_vkDebugUtilsMessengerCallbackEXT m_DebugCallback = nullptr;
    };

    using Flags = u8;
    /**
     * @brief Flags for configuring instance behavior.
     *
     * Use these flags to enable features like headless mode, validation layers,
     * or specific Vulkan extensions during instance creation.
     */
    enum FlagBits : Flags
    {
        Flag_Headless = 1 << 0,
        Flag_HasValidationLayers = 1 << 1,
        Flag_Properties2Extension = 1 << 2
    };

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

        TKit::Array64<const char *> EnabledExtensions;
        TKit::Array16<const char *> EnabledLayers;

        Vulkan::InstanceTable Table;

        u32 ApplicationVersion;
        u32 EngineVersion;
        u32 ApiVersion;

        VkDebugUtilsMessengerEXT DebugMessenger;
        const VkAllocationCallbacks *AllocationCallbacks;

        Flags Flags;
    };

    Instance() = default;
    Instance(VkInstance p_Instance, const Info &p_Info) : m_Instance(p_Instance), m_Info(p_Info)
    {
    }

    bool IsExtensionEnabled(const char *p_Extension) const;
    bool IsLayerEnabled(const char *p_Layer) const;

    void Destroy();

    VkInstance GetHandle() const
    {
        return m_Instance;
    }
    const Info &GetInfo() const
    {
        return m_Info;
    }

    Proxy CreateProxy() const;

    operator VkInstance() const
    {
        return m_Instance;
    }
    operator Proxy() const
    {
        return CreateProxy();
    }
    operator bool() const
    {
        return m_Instance != VK_NULL_HANDLE;
    }

  private:
    VkInstance m_Instance = VK_NULL_HANDLE;
    Info m_Info;
};
} // namespace VKit
