#include "tests/utils.hpp"
#include "vkit/vulkan/physical_device.hpp"

namespace VKit
{
static void CheckDevices(const TKit::Span<const FormattedResult<PhysicalDevice>> p_Devices) noexcept
{
    bool none = true;
    for (const auto &devResult : p_Devices)
    {
        if (!devResult)
        {
            WARN(devResult.GetError().Message);
            continue;
        }
        none = false;
        const PhysicalDevice &device = devResult.GetValue();

        const PhysicalDevice::Info &info = device.GetInfo();
        if (info.Flags & PhysicalDevice::Flag_Optimal)
        {
            TKIT_LOG_INFO("[VULKIT] Found optimal device: {}", info.Properties.Core.deviceName);
        }
        else
        {
            TKIT_LOG_INFO("[VULKIT] Found partially suitable device: {}", info.Properties.Core.deviceName);
        }

        TKIT_LOG_INFO("[VULKIT] Enabled extensions:");
        for (const auto &extension : info.EnabledExtensions)
            TKIT_LOG_INFO("[VULKIT]     {}", extension);
    }
    if (none)
        FAIL("No physical devices found");
}

Instance SetupInstance()
{
    SetupSystem();
    const auto result =
        Instance::Builder().RequireValidationLayers().RequestApiVersion(1, 2, 0).SetHeadless(true).Build();

    CheckResult(result);
    return result.GetValue();
}

TEST_CASE("Basic physical device enumeration", "[physical_device]")
{
    Instance instance = SetupInstance();
    const auto result = PhysicalDevice::Selector(&instance)
                            .RequireExtension("VK_KHR_swapchain")
                            .AddFlags(VKit::PhysicalDevice::Selector::Flag_AnyType)
                            .Enumerate();
    CheckResult(result);

    instance.Destroy();
    CheckDevices(result.GetValue());
}

} // namespace VKit