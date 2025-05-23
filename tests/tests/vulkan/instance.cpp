#include <catch2/catch_test_macros.hpp>
#include <vulkan/vulkan.h>
#include "vkit/vulkan/instance.hpp"
#include "vkit/vulkan/vulkan.hpp"

using namespace VKit;

TEST_CASE("Proxy default initialization", "[Instance][Proxy]")
{
    const Instance::Proxy proxy{};
    REQUIRE(static_cast<VkInstance>(proxy) == VK_NULL_HANDLE);
    REQUIRE(!static_cast<bool>(proxy));
    REQUIRE(!proxy.AllocationCallbacks);
    REQUIRE(!proxy.Table);
}

TEST_CASE("Default Instance is invalid", "[Instance]")
{
    const Instance defaultInst{};
    REQUIRE(!defaultInst);
    REQUIRE(defaultInst.GetHandle() == VK_NULL_HANDLE);
}

SCENARIO("Builder setter chaining returns the same instance reference", "[Instance][Builder]")
{
    GIVEN("A default Instance::Builder")
    {
        Instance::Builder builder;
        WHEN("Applying a chain of setters")
        {
            auto &ref = builder.SetApplicationName("TestApp")
                            .SetEngineName("TestEngine")
                            .SetApplicationVersion(1, 2, 3)
                            .SetEngineVersion(4, 5, 6)
                            .RequireApiVersion(1, 0, 0)
                            .RequestApiVersion(1, 1, 0)
                            .RequireExtension("ext1")
                            .RequestExtension("ext2")
                            .RequireLayer("layer1")
                            .RequestLayer("layer2")
                            .RequireValidationLayers()
                            .RequestValidationLayers()
                            .SetDebugCallback(nullptr)
                            .SetHeadless(true)
                            .SetDebugMessengerUserData(nullptr)
                            .SetAllocationCallbacks(nullptr);
            THEN("The returned reference should be the original builder")
            {
                REQUIRE(&ref == &builder);
            }
        }
    }
}

TEST_CASE("Headless Vulkan instance creation", "[Instance][Builder][Integration]")
{
    Instance::Builder builder;
    builder.SetApplicationName("HeadlessTestApp")
        .SetEngineName("HeadlessTestEngine")
        .SetHeadless(true)
        .RequireApiVersion(1, 0, 0)
        .RequestApiVersion(1, 1, 0);

    const auto result = builder.Build();
    VKIT_LOG_RESULT(result);
    REQUIRE(result); // Build should succeed in headless mode

    Instance instance = result.GetValue();
    REQUIRE(instance);
    REQUIRE(static_cast<VkInstance>(instance) != VK_NULL_HANDLE);

    // Check that no surface extensions are enabled
    REQUIRE_FALSE(instance.IsExtensionEnabled("VK_KHR_surface"));
    REQUIRE_FALSE(instance.IsExtensionEnabled("VK_KHR_xcb_surface"));

    instance.Destroy();
    REQUIRE(static_cast<VkInstance>(instance) == VK_NULL_HANDLE);
}
