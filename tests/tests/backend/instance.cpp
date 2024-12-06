#include "tests/utils.hpp"
#include "vkit/backend/instance.hpp"

namespace VKit
{
TEST_CASE("Minimal headless instance", "[instance][headless]")
{
    SetupSystem();
    auto result = Instance::Builder().SetHeadless().Build();
    REQUIRE(result);

    Instance &instance = result.GetValue();
    const Instance::Info &info = instance.GetInfo();

    REQUIRE(!info.ApplicationName);
    REQUIRE(!info.EngineName);
    REQUIRE(info.ApplicationVersion == VKIT_MAKE_VERSION(0, 1, 0, 0));
    REQUIRE(info.EngineVersion == VKIT_MAKE_VERSION(0, 1, 0, 0));
    REQUIRE(info.ApiVersion == VKIT_MAKE_VERSION(0, 1, 0, 0));
    REQUIRE(info.Flags & InstanceFlags_Headless);
    REQUIRE(info.EnabledLayers.size() == 0);
    REQUIRE(!info.DebugMessenger);
    REQUIRE(!info.AllocationCallbacks);

    instance.Destroy();
}

TEST_CASE("Unsupported extensions and layers", "[instance][unsupported]")
{
    SetupSystem();
    auto result = Instance::Builder().SetHeadless().RequireExtension("VK_KHR_non_existent").Build();
    REQUIRE(!result);
    REQUIRE(result.GetError().Result == VK_ERROR_EXTENSION_NOT_PRESENT);

    result = Instance::Builder().SetHeadless().RequireLayer("VK_LAYER_non_existent").Build();
    REQUIRE(!result);
    REQUIRE(result.GetError().Result == VK_ERROR_LAYER_NOT_PRESENT);
}

TEST_CASE("Validation layers", "[instance][validation]")
{
    SetupSystem();
    auto result = Instance::Builder().SetHeadless().RequestValidationLayers().Build();
    REQUIRE(result);

    Instance &instance = result.GetValue();
    const Instance::Info &info = instance.GetInfo();

    REQUIRE(info.Flags & InstanceFlags_HasValidationLayers);
    REQUIRE(info.Flags & InstanceFlags_Headless);
    REQUIRE(info.EnabledLayers.size() > 0);
    REQUIRE(info.DebugMessenger);

    instance.Destroy();
}
} // namespace VKit
