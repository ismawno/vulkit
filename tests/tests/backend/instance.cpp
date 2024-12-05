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
}
} // namespace VKit
