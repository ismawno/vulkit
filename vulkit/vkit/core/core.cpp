#define VMA_IMPLEMENTATION
#include "vkit/core/pch.hpp"
#include "vkit/core/core.hpp"
#include "vkit/descriptors/descriptor_pool.hpp"
#include "vkit/descriptors/descriptor_set_layout.hpp"
#include "tkit/core/logging.hpp"
#include "tkit/container/storage.hpp"
#include "tkit/memory/stack_allocator.hpp"

namespace VKit
{
static TKit::StackAllocator *s_StackAllocator;

static TKit::Ref<Instance> s_Instance;
static TKit::Ref<Device> s_Device;

static VmaAllocator s_VulkanAllocator = VK_NULL_HANDLE;

static void createVulkanAllocator() noexcept
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = s_Device->GetPhysicalDevice();
    allocatorInfo.device = s_Device->GetDevice();
    allocatorInfo.instance = s_Instance->GetInstance();
#ifdef TKIT_OS_APPLE
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0;
#else
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
#endif
    allocatorInfo.flags = 0;
    allocatorInfo.pVulkanFunctions = nullptr;
    TKIT_ASSERT_RETURNS(vmaCreateAllocator(&allocatorInfo, &s_VulkanAllocator), VK_SUCCESS,
                        "Failed to create vulkan allocator");
}

void Core::Initialize(const Specs &p_Specs) noexcept
{
    s_StackAllocator = new TKit::StackAllocator(p_Specs.StackAllocatorMemory);
    s_Instance = TKit::Ref<Instance>::Create(p_Specs.Instance);
}

void Core::Terminate() noexcept
{
    if (s_Device)
        DestroyCombinedPrimitiveBuffers();
    glfwTerminate();
    if (s_Device)
        s_Device->WaitIdle();
    vmaDestroyAllocator(s_VulkanAllocator);
    // Release all refcounted objects. After this call, none are guaranteed to be valid
    s_TransformStorageLayout.Destroy();
    s_LightStorageLayout.Destroy();

    vkDestroyPipelineLayout(s_Device->GetDevice(), s_GraphicsPipelineLayout2D, nullptr);
    vkDestroyPipelineLayout(s_Device->GetDevice(), s_GraphicsPipelineLayout3D, nullptr);

    s_DescriptorPool.Destroy();
    s_Device = nullptr;
    s_Instance = nullptr;
}

const TKit::Ref<Instance> &Core::GetInstance() noexcept
{
    TKIT_ASSERT(s_Instance, "Vulkan instance is not initialize! Forgot to call Onyx::Core::Initialize?");
    return s_Instance;
}
const TKit::Ref<Device> &Core::GetDevice() noexcept
{
    return s_Device;
}

VmaAllocator Core::GetVulkanAllocator() noexcept
{
    return s_VulkanAllocator;
}

const DescriptorPool *Core::GetDescriptorPool() noexcept
{
    return s_DescriptorPool.Get();
}
const DescriptorSetLayout *Core::GetTransformStorageDescriptorSetLayout() noexcept
{
    return s_TransformStorageLayout.Get();
}
const DescriptorSetLayout *Core::GetLightStorageDescriptorSetLayout() noexcept
{
    return s_LightStorageLayout.Get();
}

template <Dimension D> VkPipelineLayout Core::GetGraphicsPipelineLayout() noexcept
{
    if constexpr (D == D2)
        return s_GraphicsPipelineLayout2D;
    else
        return s_GraphicsPipelineLayout3D;
}

TKit::StackAllocator *Core::GetStackAllocator() noexcept
{
    return s_StackAllocator;
}
TKit::ITaskManager *Core::GetTaskManager() noexcept
{
    return s_Manager;
}

const TKit::Ref<Device> &Core::tryCreateDevice(VkSurfaceKHR p_Surface) noexcept
{
    if (!s_Device)
    {
        s_Device = TKit::Ref<Device>::Create(p_Surface);
        createVulkanAllocator();
        createDescriptorData();
        createPipelineLayouts();
        CreateCombinedPrimitiveBuffers();
    }
    TKIT_ASSERT(s_Device->IsSuitable(p_Surface), "The current device is not suitable for the given surface");
    return s_Device;
}

template VkPipelineLayout Core::GetGraphicsPipelineLayout<D2>() noexcept;
template VkPipelineLayout Core::GetGraphicsPipelineLayout<D3>() noexcept;

} // namespace VKit
