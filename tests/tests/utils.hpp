#pragma once

#include "vkit/vulkan/system.hpp"
#include <catch2/catch_test_macros.hpp>

namespace VKit
{
void SetupSystem() noexcept;

template <typename T> void CheckResult(const T &result) noexcept
{
    if (!result)
        FAIL(result.GetError().Message);
}

template <typename T> void CheckVulkanResult(const T &result) noexcept
{
    if (!result)
        FAIL(result.Message);
}
} // namespace VKit