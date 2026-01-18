#pragma once

#ifndef VKIT_ENABLE_INSTANCE
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_INSTANCE"
#endif

#include "vkit/vulkan/loader.hpp"
#include "vkit/core/alias.hpp"
#include "tkit/container/span.hpp"
#include <vulkan/vulkan.h>

namespace VKit
{
using InstanceFlags = u8;
enum InstanceFlagBits : InstanceFlags
{
    InstanceFlag_Headless = 1 << 0,
    InstanceFlag_HasValidationLayers = 1 << 1,
    InstanceFlag_Properties2Extension = 1 << 2
};

class Instance
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

    class Builder
    {
      public:
        Builder();

        VKIT_NO_DISCARD Result<Instance> Build() const;

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
        Builder &RequestExtension(const char *p_Extension);

        Builder &RequireLayer(const char *p_Layer);
        Builder &RequestLayer(const char *p_Layer);

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

        TKit::ArenaArray<const char *> m_RequiredExtensions{};
        TKit::ArenaArray<const char *> m_RequestedExtensions{};

        TKit::ArenaArray<const char *> m_RequiredLayers{};
        TKit::ArenaArray<const char *> m_RequestedLayers{};

        bool m_RequireValidationLayers = false;
        bool m_RequestValidationLayers = false;
        bool m_Headless = false;

        void *m_DebugMessengerUserData = nullptr;
        const VkAllocationCallbacks *m_AllocationCallbacks = nullptr;

        PFN_vkDebugUtilsMessengerCallbackEXT m_DebugCallback = nullptr;
    };

    struct Info
    {
        const char *ApplicationName;
        const char *EngineName;

        TKit::ArenaArray<const char *> EnabledExtensions;
        TKit::ArenaArray<const char *> EnabledLayers;

        Vulkan::InstanceTable Table;

        u32 ApplicationVersion;
        u32 EngineVersion;
        u32 ApiVersion;

        VkDebugUtilsMessengerEXT DebugMessenger;
        const VkAllocationCallbacks *AllocationCallbacks;

        InstanceFlags Flags;
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
