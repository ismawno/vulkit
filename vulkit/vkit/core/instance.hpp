#pragma once

#include "vkit/core/api.hpp"
#include "tkit/memory/ptr.hpp"
#include <vulkan/vulkan.hpp>

namespace VKit
{
class VKIT_API Instance : public TKit::RefCounted<Instance>
{
    TKIT_NON_COPYABLE(Instance)
    struct Specs
    {
        const char *ApplicationName = "Vulkan app";
        const char *EngineName = "No Engine";
        u32 ApplicationVersion = VK_MAKE_VERSION(1, 0, 0);
        u32 EngineVersion = VK_MAKE_VERSION(1, 0, 0);
        u32 ApiVersion = VK_API_VERSION_1_0;
        std::span<const char *> RequiredExtensions;

        VkInstanceCreateFlags Flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#ifdef VKIT_VALIDATION_LAYERS
        PFN_vkDebugUtilsMessengerCallbackEXT DebugCallback = nullptr;
#endif
    };

  public:
    Instance(const Specs &p_Specs) noexcept;
    ~Instance() noexcept;

#ifdef TKIT_ENABLE_ASSERTS
    static const char *GetValidationLayer() noexcept;
#endif

    VkInstance GetInstance() const noexcept;

  private:
    void createInstance(const Specs &p_Specs) noexcept;

    VkInstance m_Instance;

    friend struct Core;
};
} // namespace VKit