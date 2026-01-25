#pragma once

#ifndef VKIT_ENABLE_INSTANCE
#    error                                                                                                             \
        "[VULKIT] To include this file, the corresponding feature must be enabled in CMake with VULKIT_ENABLE_INSTANCE"
#endif

#include "vkit/vulkan/loader.hpp"
#include "vkit/core/alias.hpp"
#include "tkit/container/tier_array.hpp"
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

        Builder &SetApplicationName(const char *name);
        Builder &SetEngineName(const char *name);

        Builder &SetApplicationVersion(u32 version);
        Builder &SetEngineVersion(u32 version);

        Builder &SetApplicationVersion(u32 major, u32 minor, u32 patch);
        Builder &SetEngineVersion(u32 major, u32 minor, u32 patch);

        Builder &RequireApiVersion(u32 version);
        Builder &RequireApiVersion(u32 major, u32 minor, u32 patch);

        Builder &RequestApiVersion(u32 version);
        Builder &RequestApiVersion(u32 major, u32 minor, u32 patch);

        Builder &RequireExtension(const char *extension);
        Builder &RequestExtension(const char *extension);

        Builder &RequireLayer(const char *layer);
        Builder &RequestLayer(const char *layer);

        Builder &RequireValidationLayers();
        Builder &RequestValidationLayers();

        Builder &SetDebugCallback(PFN_vkDebugUtilsMessengerCallbackEXT callback);
        Builder &SetHeadless(bool headless = true);

        Builder &SetDebugMessengerUserData(void *data);
        Builder &SetAllocationCallbacks(const VkAllocationCallbacks *allocationCallbacks);

      private:
        const char *m_ApplicationName = nullptr;
        const char *m_EngineName = nullptr;

        u32 m_ApplicationVersion = VKIT_MAKE_VERSION(0, 1, 0, 0);
        u32 m_EngineVersion = VKIT_MAKE_VERSION(0, 1, 0, 0);
        u32 m_RequiredApiVersion = VKIT_MAKE_VERSION(0, 1, 0, 0);
        u32 m_RequestedApiVersion = VKIT_MAKE_VERSION(0, 1, 0, 0);

        TKit::TierArray<const char *> m_RequiredExtensions{};
        TKit::TierArray<const char *> m_RequestedExtensions{};

        TKit::TierArray<const char *> m_RequiredLayers{};
        TKit::TierArray<const char *> m_RequestedLayers{};

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

        TKit::TierArray<const char *> EnabledExtensions;
        TKit::TierArray<const char *> EnabledLayers;

        Vulkan::InstanceTable *Table;

        u32 ApplicationVersion;
        u32 EngineVersion;
        u32 ApiVersion;

        VkDebugUtilsMessengerEXT DebugMessenger;
        const VkAllocationCallbacks *AllocationCallbacks;

        InstanceFlags Flags;
    };

    Instance() = default;
    Instance(VkInstance instance, const Info &info) : m_Instance(instance), m_Info(info)
    {
    }

    bool IsExtensionEnabled(const char *extension) const;
    bool IsLayerEnabled(const char *layer) const;

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
