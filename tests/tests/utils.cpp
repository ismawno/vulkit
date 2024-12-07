#include "tests/utils.hpp"

namespace VKit
{
void SetupSystem() noexcept
{
    const auto sysres = System::Initialize();
    REQUIRE(sysres);
}
} // namespace VKit