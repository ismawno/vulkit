#include "tests/utils.hpp"

namespace VKit
{
void Setup() noexcept
{
    const auto vkres = Core::Initialize();
    REQUIRE(vkres);
}
} // namespace VKit
