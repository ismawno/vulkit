#pragma once

#include "vkit/backend/system.hpp"
#include <catch2/catch_test_macros.hpp>

namespace VKit
{
void SetupSystem()
{
    const auto sysres = System::Initialize();
    REQUIRE(sysres);
}
} // namespace VKit