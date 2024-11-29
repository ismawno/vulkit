#pragma once

#include "vkit/core/api.hpp"
#include "vkit/core/alias.hpp"
#include "vkit/backend/system.hpp"
#include "tkit/memory/ptr.hpp"
#include "tkit/utilities/result.hpp"
#include <vulkan/vulkan.hpp>

namespace VKit
{
class VKIT_API Instance : public TKit::RefCounted<Instance>
{
  public:
    struct Info
    {
        const char *ApplicationName;
        const char *EngineName;

        u32 ApplicationVersion;
        u32 EngineVersion;
        u32 ApiVersion;

        bool HasValidationLayers;
        bool Properties2Extension;
        bool Headless;

        DynamicArray<const char *> Extensions;
        DynamicArray<const char *> Layers;

        VkDebugUtilsMessengerEXT DebugMessenger;
        const VkAllocationCallbacks *AllocationCallbacks;
    };

    class Specs
    {
      public:
        Result<Instance> Create() const noexcept;

        Specs &SetApplicationName(const char *p_Name) noexcept;
        Specs &SetEngineName(const char *p_Name) noexcept;

        Specs &SetApplicationVersion(u32 p_Version) noexcept;
        Specs &SetEngineVersion(u32 p_Version) noexcept;

        Specs &SetApplicationVersion(u32 p_Major, u32 p_Minor, u32 p_Patch) noexcept;
        Specs &SetEngineVersion(u32 p_Major, u32 p_Minor, u32 p_Patch) noexcept;

        Specs &RequireApiVersion(u32 p_Version) noexcept;
        Specs &RequireApiVersion(u32 p_Major, u32 p_Minor, u32 p_Patch) noexcept;

        Specs &RequestApiVersion(u32 p_Version) noexcept;
        Specs &RequestApiVersion(u32 p_Major, u32 p_Minor, u32 p_Patch) noexcept;

        Specs &RequireExtension(const char *p_Extension) noexcept;
        Specs &RequireExtensions(std::span<const char *const> p_Extensions) noexcept;

        Specs &RequestExtension(const char *p_Extension) noexcept;
        Specs &RequestExtensions(std::span<const char *const> p_Extensions) noexcept;

        Specs &RequireLayer(const char *p_Layer) noexcept;
        Specs &RequireLayers(std::span<const char *const> p_Layers) noexcept;

        Specs &RequestLayer(const char *p_Layer) noexcept;
        Specs &RequestLayers(std::span<const char *const> p_Layers) noexcept;

        Specs &RequireValidationLayers() noexcept;
        Specs &RequestValidationLayers() noexcept;

        Specs &SetDebugCallback(PFN_vkDebugUtilsMessengerCallbackEXT p_Callback) noexcept;
        Specs &SetHeadless(bool p_Headless = true) noexcept;

        Specs &SetDebugMessengerUserData(void *p_Data) noexcept;
        Specs &SetAllocationCallbacks(const VkAllocationCallbacks *p_AllocationCallbacks) noexcept;

      private:
        const char *m_ApplicationName = nullptr;
        const char *m_EngineName = nullptr;

        u32 m_ApplicationVersion = VK_API_VERSION_1_0;
        u32 m_EngineVersion = VK_API_VERSION_1_0;
        u32 m_RequiredApiVersion = VK_API_VERSION_1_0;
        u32 m_RequestedApiVersion = VK_API_VERSION_1_0;

        DynamicArray<const char *> m_RequiredExtensions;
        DynamicArray<const char *> m_RequestedExtensions;

        DynamicArray<const char *> m_RequiredLayers;
        DynamicArray<const char *> m_RequestedLayers;

        bool m_RequireValidationLayers = false;
        bool m_RequestValidationLayers = false;
        bool m_Headless = false;

        void *m_DebugMessengerUserData = nullptr;
        const VkAllocationCallbacks *m_AllocationCallbacks = nullptr;

        PFN_vkDebugUtilsMessengerCallbackEXT m_DebugCallback = nullptr;
    };

    static void Destroy(const Instance &p_Instance) noexcept;

    VkInstance GetInstance() const noexcept;
    const Info &GetInfo() const noexcept;

    explicit(false) operator VkInstance() const noexcept;

  private:
    Instance(VkInstance p_Instance, const Info &p_Info) noexcept;

    VkInstance m_Instance;
    Info m_Info{};
};
} // namespace VKit