#include "vkit/backend/instance.hpp"
#include <catch2/catch_test_macros.hpp>

namespace VKit
{
TEST_CASE("Minimal headless instance", "[instance][headless]")
{
    const auto sysres = System::Initialize();
    REQUIRE(sysres);

    const auto result = Instance::Builder().SetHeadless().Build();
    REQUIRE(result);

    const Instance &instance = result.GetValue();
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
}
} // namespace VKit
