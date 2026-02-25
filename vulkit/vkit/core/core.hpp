#pragma once

#include "vkit/vulkan/vulkan.hpp"
#include "tkit/memory/arena_allocator.hpp"
#include "tkit/memory/stack_allocator.hpp"
#include "tkit/memory/tier_allocator.hpp"

namespace VKit
{
using Flags = u8;
struct Allocation
{
    TKit::ArenaAllocator *Arena = nullptr;
    TKit::StackAllocator *Stack = nullptr;
    TKit::TierAllocator *Tier = nullptr;
};
struct Specs
{
    const char *LoaderPath = nullptr;
    Allocation Allocators{};
};

VKIT_NO_DISCARD Result<> Initialize(const Specs &specs = {});
void Terminate();

} // namespace VKit

namespace VKit::Core
{
bool IsExtensionSupported(const char *name);
bool IsLayerSupported(const char *name);

const VkExtensionProperties *GetExtensionByName(const char *name);
const VkLayerProperties *GetLayerByName(const char *name);

const VkExtensionProperties &GetExtensionByIndex(u32 index);
const VkLayerProperties &GetLayerByIndex(u32 index);

u32 GetExtensionCount();
u32 GetLayerCount();

} // namespace VKit::Core
