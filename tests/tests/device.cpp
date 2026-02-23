/**
 * @file test_device_creation.cpp
 * @brief Comprehensive Catch2 test suite for VKit Core, Instance, PhysicalDevice, and LogicalDevice
 *
 * Tests the full Vulkan initialization pipeline from library loading through device creation.
 */

#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "vkit/core/core.hpp"
#include "vkit/vulkan/instance.hpp"
#include "vkit/device/physical_device.hpp"
#include "vkit/device/logical_device.hpp"
#include "vkit/execution/queue.hpp"
TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_CLANG_WARNING_IGNORE("-Wunused-parameter")
TKIT_GCC_WARNING_IGNORE("-Wunused-parameter")
#include <catch2/catch_all.hpp>
TKIT_COMPILER_WARNING_IGNORE_POP()

#include <cstring>

using namespace TKit::Alias;

namespace
{
struct WarningShutter : Catch::EventListenerBase
{
    using EventListenerBase::EventListenerBase;

    void testCaseStarting(const Catch::TestCaseInfo &) override
    {
        TKIT_LOGS_DISABLE(TKIT_DEBUG_LOGS_BIT | TKIT_INFO_LOGS_BIT | TKIT_WARNING_LOGS_BIT);
    }

    void testCaseEnded(const Catch::TestCaseStats &) override
    {
        TKIT_LOGS_ENABLE(TKIT_INFO_LOGS_BIT | TKIT_WARNING_LOGS_BIT);
    }
};

CATCH_REGISTER_LISTENER(WarningShutter)

// ============================================================================
// Test Utilities
// ============================================================================

/**
 * @brief RAII guard for Core initialization/termination
 */
class CoreGuard
{
  public:
    CoreGuard()
    {
        auto result = VKit::Core::Initialize();
        m_Valid = static_cast<bool>(result);
    }

    ~CoreGuard()
    {
        if (m_Valid)
            VKit::Core::Terminate();
    }

    bool IsValid() const
    {
        return m_Valid;
    }

  private:
    bool m_Valid = false;
};

/**
 * @brief RAII guard for Instance with automatic cleanup
 */
class InstanceGuard
{
  public:
    InstanceGuard(VKit::Instance &&instance) : m_Instance(std::move(instance))
    {
    }

    ~InstanceGuard()
    {
        m_Instance.Destroy();
    }

    VKit::Instance &Get()
    {
        return m_Instance;
    }

    const VKit::Instance &Get() const
    {
        return m_Instance;
    }

    operator VKit::Instance &()
    {
        return m_Instance;
    }

  private:
    VKit::Instance m_Instance;
};

/**
 * @brief Creates a minimal headless instance for testing
 */
VKit::Result<VKit::Instance> CreateMinimalInstance()
{
    return VKit::Instance::Builder()
        .SetApplicationName("VKit Test")
        .SetApplicationVersion(1, 0, 0)
        .RequireApiVersion(1, 0, 0)
        .SetHeadless(true)
        .Build();
}

/**
 * @brief Creates an instance with validation layers for testing
 */
VKit::Result<VKit::Instance> CreateValidatedInstance()
{
    return VKit::Instance::Builder()
        .SetApplicationName("VKit Validated Test")
        .SetApplicationVersion(1, 0, 0)
        .SetEngineName("VKit Test Engine")
        .SetEngineVersion(1, 0, 0)
        .RequireApiVersion(1, 0, 0)
        .RequestApiVersion(1, 2, 0)
        .RequireLayer("VK_LAYER_KHRONOS_validation")
        .SetHeadless(true)
        .Build();
}

} // anonymous namespace

// ============================================================================
// CORE TESTS
// ============================================================================

TEST_CASE("Core::Initialize - Basic Initialization", "[core][init]")
{
    SECTION("Initialize succeeds on supported system")
    {
        const auto result = VKit::Core::Initialize();
        REQUIRE(result);

        VKit::Core::Terminate();
    }

    SECTION("Double initialization is safe (idempotent)")
    {
        const auto result1 = VKit::Core::Initialize();
        REQUIRE(result1);

        const auto result2 = VKit::Core::Initialize();
        REQUIRE(result2);

        VKit::Core::Terminate();
    }

    SECTION("Initialize populates available extensions")
    {
        const auto result = VKit::Core::Initialize();
        REQUIRE(result);

        // Should have at least some extensions available
        CHECK_FALSE(VKit::Core::GetExtensionCount() == 0);

        VKit::Core::Terminate();
    }
}

TEST_CASE("Core::Terminate", "[core][terminate]")
{
    SECTION("Terminate after initialization")
    {
        const auto result = VKit::Core::Initialize();
        REQUIRE(result);

        VKit::Core::Terminate();
        // Should not crash
    }

    SECTION("Double terminate is safe")
    {
        const auto result = VKit::Core::Initialize();
        REQUIRE(result);

        VKit::Core::Terminate();
        VKit::Core::Terminate(); // Should not crash
    }

    SECTION("Terminate without initialization is safe")
    {
        VKit::Core::Terminate(); // Should not crash
    }
}

TEST_CASE("Core::IsExtensionSupported", "[core][extensions]")
{
    CoreGuard guard;
    REQUIRE(guard.IsValid());

    SECTION("VK_KHR_surface is commonly available")
    {
        // This extension is available on most systems with display capability
        bool supported = VKit::Core::IsExtensionSupported("VK_KHR_surface");
        // May or may not be supported depending on headless setup
        INFO("VK_KHR_surface supported: " << supported);
    }

    SECTION("Non-existent extension returns false")
    {
        bool supported = VKit::Core::IsExtensionSupported("VK_FAKE_nonexistent_extension_12345");
        CHECK_FALSE(supported);
    }

    SECTION("Empty string returns false")
    {
        bool supported = VKit::Core::IsExtensionSupported("");
        CHECK_FALSE(supported);
    }

    SECTION("GetExtension returns nullptr for unsupported")
    {
        const VkExtensionProperties *ext = VKit::Core::GetExtensionByName("VK_FAKE_nonexistent_extension");
        CHECK(ext == nullptr);
    }

    SECTION("GetExtension returns valid pointer for supported extension")
    {
        // Find any available extension
        if (VKit::Core::GetExtensionCount() != 0)
        {
            const char *extName = VKit::Core::GetExtensionByIndex(0).extensionName;
            const VkExtensionProperties *ext = VKit::Core::GetExtensionByName(extName);
            REQUIRE(ext != nullptr);
            CHECK(std::strcmp(ext->extensionName, extName) == 0);
        }
    }
}

TEST_CASE("Core::IsLayerSupported", "[core][layers]")
{
    CoreGuard guard;
    REQUIRE(guard.IsValid());

    SECTION("Non-existent layer returns false")
    {
        bool supported = VKit::Core::IsLayerSupported("VK_LAYER_FAKE_nonexistent_12345");
        CHECK_FALSE(supported);
    }

    SECTION("Empty string returns false")
    {
        bool supported = VKit::Core::IsLayerSupported("");
        CHECK_FALSE(supported);
    }

    SECTION("VK_LAYER_KHRONOS_validation may be available")
    {
        bool supported = VKit::Core::IsLayerSupported("VK_LAYER_KHRONOS_validation");
        INFO("VK_LAYER_KHRONOS_validation supported: " << supported);
        // Just logging - availability depends on SDK installation
    }

    SECTION("GetLayer returns nullptr for unsupported")
    {
        const VkLayerProperties *layer = VKit::Core::GetLayerByName("VK_LAYER_FAKE_nonexistent");
        CHECK(layer == nullptr);
    }

    SECTION("GetLayer returns valid pointer for supported layer")
    {
        if (VKit::Core::GetLayerCount() != 0)
        {
            const char *layerName = VKit::Core::GetLayerByIndex(0).layerName;
            const VkLayerProperties *layer = VKit::Core::GetLayerByName(layerName);
            REQUIRE(layer != nullptr);
            CHECK(std::strcmp(layer->layerName, layerName) == 0);
        }
    }
}

// ============================================================================
// INSTANCE BUILDER TESTS
// ============================================================================

TEST_CASE("Instance::Builder - Basic Creation", "[instance][builder][create]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    SECTION("Minimal headless instance creation")
    {
        const auto result = VKit::Instance::Builder().SetApplicationName("Test App").SetHeadless(true).Build();

        REQUIRE(result);

        VKit::Instance instance = result.GetValue();
        CHECK(instance.GetHandle() != VK_NULL_HANDLE);

        instance.Destroy();
    }

    SECTION("Full configuration headless instance")
    {
        const auto result = VKit::Instance::Builder()
                                .SetApplicationName("Full Test App")
                                .SetApplicationVersion(2, 1, 3)
                                .SetEngineName("Test Engine")
                                .SetEngineVersion(1, 0, 0)
                                .RequireApiVersion(1, 0, 0)
                                .SetHeadless(true)
                                .Build();

        REQUIRE(result);

        VKit::Instance instance = result.GetValue();
        const auto &info = instance.GetInfo();

        CHECK(std::strcmp(info.ApplicationName, "Full Test App") == 0);
        CHECK(std::strcmp(info.EngineName, "Test Engine") == 0);
        CHECK(info.ApiVersion >= VKIT_MAKE_VERSION(0, 1, 0, 0));

        instance.Destroy();
    }

    SECTION("Instance with validation layers requested")
    {
        const auto result = VKit::Instance::Builder()
                                .SetApplicationName("Validated App")
                                .SetHeadless(true)
                                .RequireLayer("VK_LAYER_KHRONOS_validation")
                                .Build();

        REQUIRE(result);

        VKit::Instance instance = result.GetValue();
        instance.Destroy();
    }
}

TEST_CASE("Instance::Builder - API Version Handling", "[instance][builder][version]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    SECTION("Require API version 1.0.0 succeeds")
    {
        const auto result = VKit::Instance::Builder()
                                .SetApplicationName("Version Test")
                                .RequireApiVersion(1, 0, 0)
                                .SetHeadless(true)
                                .Build();

        REQUIRE(result);

        VKit::Instance instance = result.GetValue();
        CHECK(instance.GetInfo().ApiVersion >= VKIT_MAKE_VERSION(0, 1, 0, 0));

        instance.Destroy();
    }

    SECTION("Request higher API version with fallback")
    {
        const auto result = VKit::Instance::Builder()
                                .SetApplicationName("Version Fallback Test")
                                .RequireApiVersion(1, 0, 0)
                                .RequestApiVersion(1, 3, 0) // May not be available
                                .SetHeadless(true)
                                .Build();

        REQUIRE(result);

        VKit::Instance instance = result.GetValue();
        // Should get at least 1.0.0
        CHECK(instance.GetInfo().ApiVersion >= VKIT_MAKE_VERSION(0, 1, 0, 0));

        instance.Destroy();
    }

    SECTION("Requiring impossibly high version fails gracefully")
    {
        const auto result = VKit::Instance::Builder()
                                .SetApplicationName("High Version Test")
                                .RequireApiVersion(99, 99, 99) // Impossibly high
                                .SetHeadless(true)
                                .Build();

        CHECK_FALSE(result);
        if (!result)
        {
            CHECK(result.GetError().GetCode() == VKit::Error_VersionMismatch);
        }
    }
}

TEST_CASE("Instance::Builder - Extension Handling", "[instance][builder][extensions]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    SECTION("Request available extension")
    {
        // Find an available extension to request
        if (VKit::Core::GetExtensionCount() == 0)
        {
            SKIP("No extensions available for testing");
        }

        const char *extName = VKit::Core::GetExtensionByIndex(0).extensionName;
        const auto result = VKit::Instance::Builder()
                                .SetApplicationName("Extension Test")
                                .RequestExtension(extName)
                                .SetHeadless(true)
                                .Build();

        REQUIRE(result);

        VKit::Instance instance = result.GetValue();
        CHECK(instance.IsExtensionEnabled(extName));

        instance.Destroy();
    }

    SECTION("Request unavailable extension does not fail")
    {
        const auto result = VKit::Instance::Builder()
                                .SetApplicationName("Missing Extension Test")
                                .RequestExtension("VK_FAKE_extension_does_not_exist")
                                .SetHeadless(true)
                                .Build();

        // Should succeed - requested extensions are optional
        REQUIRE(result);

        VKit::Instance instance = result.GetValue();
        CHECK_FALSE(instance.IsExtensionEnabled("VK_FAKE_extension_does_not_exist"));

        instance.Destroy();
    }

    SECTION("Require unavailable extension fails")
    {
        const auto result = VKit::Instance::Builder()
                                .SetApplicationName("Required Extension Test")
                                .RequireExtension("VK_FAKE_extension_does_not_exist")
                                .SetHeadless(true)
                                .Build();

        CHECK_FALSE(result);
        if (!result)
        {
            CHECK(result.GetError().GetCode() == VKit::Error_MissingExtension);
        }
    }
}

TEST_CASE("Instance::Builder - Layer Handling", "[instance][builder][layers]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    SECTION("Request unavailable layer does not fail")
    {
        const auto result = VKit::Instance::Builder()
                                .SetApplicationName("Missing Layer Test")
                                .RequestLayer("VK_LAYER_FAKE_does_not_exist")
                                .SetHeadless(true)
                                .Build();

        REQUIRE(result);

        VKit::Instance instance = result.GetValue();
        CHECK_FALSE(instance.IsLayerEnabled("VK_LAYER_FAKE_does_not_exist"));

        instance.Destroy();
    }

    SECTION("Require unavailable layer fails")
    {
        const auto result = VKit::Instance::Builder()
                                .SetApplicationName("Required Layer Test")
                                .RequireLayer("VK_LAYER_FAKE_does_not_exist")
                                .SetHeadless(true)
                                .Build();

        CHECK_FALSE(result);
        if (!result)
        {
            CHECK(result.GetError().GetCode() == VKit::Error_MissingLayer);
        }
    }

    SECTION("Request validation layers")
    {
        const auto result = VKit::Instance::Builder()
                                .SetApplicationName("Validation Layer Test")
                                .RequireLayer("VK_LAYER_KHRONOS_validation")
                                .SetHeadless(true)
                                .Build();

        REQUIRE(result);

        VKit::Instance instance = result.GetValue();
        bool hasValidation = instance.IsLayerEnabled("VK_LAYER_KHRONOS_validation");
        INFO("Validation layer enabled: " << hasValidation);

        instance.Destroy();
    }
}

TEST_CASE("Instance - Destruction", "[instance][destroy]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    SECTION("Destroy sets handle to null")
    {
        const auto result = CreateMinimalInstance();
        REQUIRE(result);

        VKit::Instance instance = result.GetValue();
        CHECK(instance.GetHandle() != VK_NULL_HANDLE);

        instance.Destroy();
        CHECK(instance.GetHandle() == VK_NULL_HANDLE);
    }

    SECTION("Double destroy is safe")
    {
        const auto result = CreateMinimalInstance();
        REQUIRE(result);

        VKit::Instance instance = result.GetValue();
        instance.Destroy();
        instance.Destroy(); // Should not crash

        CHECK(instance.GetHandle() == VK_NULL_HANDLE);
    }
}

TEST_CASE("Instance - Proxy Creation", "[instance][proxy]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    const auto result = CreateMinimalInstance();
    REQUIRE(result);

    VKit::Instance instance = result.GetValue();

    SECTION("CreateProxy returns valid proxy")
    {
        auto proxy = instance.CreateProxy();

        CHECK(proxy.Instance == instance.GetHandle());
        CHECK(proxy.Table != nullptr);
    }

    instance.Destroy();
}

// ============================================================================
// PHYSICAL DEVICE SELECTOR TESTS
// ============================================================================

TEST_CASE("PhysicalDevice::Selector - Basic Selection", "[physical_device][selector]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    auto instanceResult = CreateValidatedInstance();
    REQUIRE(instanceResult);
    InstanceGuard instanceGuard(std::move(instanceResult.GetValue()));

    SECTION("Select any device succeeds on systems with GPU")
    {
        const auto result = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                                .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                                .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                                .Select();

        if (!result)
        {
            // No GPU available - skip rest of test
            WARN("No physical device available: " << result.GetError().GetMessage());
            return;
        }

        VKit::PhysicalDevice device = result.GetValue();
        CHECK(device.GetHandle() != VK_NULL_HANDLE);
    }

    SECTION("Enumerate returns all available devices")
    {
        const auto result = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                                .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                                .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                                .Enumerate();

        if (!result)
        {
            WARN("Failed to enumerate devices: " << result.GetError().GetMessage());
            return;
        }

        const auto &devices = result.GetValue();
        CHECK_FALSE(devices.IsEmpty());

        INFO("Found " << devices.GetSize() << " physical device(s)");
    }
}

TEST_CASE("PhysicalDevice::Selector - Device GetCode() Preference", "[physical_device][selector][type]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    auto instanceResult = CreateValidatedInstance();
    REQUIRE(instanceResult);
    InstanceGuard instanceGuard(std::move(instanceResult.GetValue()));

    SECTION("Prefer discrete GPU")
    {
        const auto result = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                                .PreferType(VKit::Device_Discrete)
                                .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                                .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                                .Select();

        if (result)
        {
            VKit::PhysicalDevice device = result.GetValue();
            INFO("Selected device type: " << static_cast<int>(device.GetInfo().Type));

            // If discrete is available, it should be selected
            // Otherwise falls back to whatever is available
        }
    }

    SECTION("Prefer integrated GPU")
    {
        const auto result = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                                .PreferType(VKit::Device_Integrated)
                                .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                                .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                                .Select();

        if (result)
        {
            VKit::PhysicalDevice device = result.GetValue();
            INFO("Selected device type: " << static_cast<int>(device.GetInfo().Type));
        }
    }

    SECTION("Strict type requirement without AnyType flag")
    {
        const auto result = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                                .PreferType(VKit::Device_Virtual) // Unlikely to exist
                                .RemoveFlags(VKit::DeviceSelectorFlag_AnyType)
                                .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                                .Select();

        // May fail if no virtual GPU exists
        if (!result)
        {
            CHECK(result.GetError().GetCode() == VKit::Error_RejectedDevice);
        }
    }
}

TEST_CASE("PhysicalDevice::Selector - Queue Requirements", "[physical_device][selector][queues]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    auto instanceResult = CreateValidatedInstance();
    REQUIRE(instanceResult);
    InstanceGuard instanceGuard(std::move(instanceResult.GetValue()));

    SECTION("Require graphics queue")
    {
        const auto result = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                                .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                                .AddFlags(VKit::DeviceSelectorFlag_RequireGraphicsQueue)
                                .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                                .Select();

        if (result)
        {
            VKit::PhysicalDevice device = result.GetValue();
            CHECK((device.GetInfo().Flags & VKit::DeviceFlag_HasGraphicsQueue) != 0);
            CHECK(device.GetInfo().FamilyIndices[VKit::Queue_Graphics] != TKIT_U32_MAX);
        }
    }

    SECTION("Require compute queue")
    {
        const auto result = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                                .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                                .AddFlags(VKit::DeviceSelectorFlag_RequireComputeQueue)
                                .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                                .Select();

        if (result)
        {
            VKit::PhysicalDevice device = result.GetValue();
            CHECK((device.GetInfo().Flags & VKit::DeviceFlag_HasComputeQueue) != 0);
        }
    }

    SECTION("Require transfer queue")
    {
        const auto result = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                                .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                                .AddFlags(VKit::DeviceSelectorFlag_RequireTransferQueue)
                                .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                                .Select();

        if (result)
        {
            VKit::PhysicalDevice device = result.GetValue();
            CHECK((device.GetInfo().Flags & VKit::DeviceFlag_HasTransferQueue) != 0);
        }
    }

    SECTION("Require dedicated compute queue")
    {
        const auto result = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                                .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                                .AddFlags(VKit::DeviceSelectorFlag_RequireDedicatedComputeQueue)
                                .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                                .Select();

        if (result)
        {
            VKit::PhysicalDevice device = result.GetValue();
            CHECK((device.GetInfo().Flags & VKit::DeviceFlag_HasDedicatedComputeQueue) != 0);
        }
        else
        {
            // May not have dedicated compute queue
            CHECK(result.GetError().GetCode() == VKit::Error_MissingQueue);
        }
    }

    SECTION("Require separate transfer queue")
    {
        const auto result = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                                .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                                .AddFlags(VKit::DeviceSelectorFlag_RequireSeparateTransferQueue)
                                .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                                .Select();

        if (result)
        {
            VKit::PhysicalDevice device = result.GetValue();
            CHECK((device.GetInfo().Flags & VKit::DeviceFlag_HasSeparateTransferQueue) != 0);
        }
        else
        {
            CHECK(result.GetError().GetCode() == VKit::Error_MissingQueue);
        }
    }
}

TEST_CASE("PhysicalDevice::Selector - Extension Requirements", "[physical_device][selector][extensions]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    auto instanceResult = CreateValidatedInstance();
    REQUIRE(instanceResult);
    InstanceGuard instanceGuard(std::move(instanceResult.GetValue()));

    SECTION("Require unavailable extension fails")
    {
        const auto result = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                                .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                                .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                                .RequireExtension("VK_FAKE_does_not_exist")
                                .Select();

        CHECK_FALSE(result);
        if (!result)
        {
            CHECK(result.GetError().GetCode() == VKit::Error_MissingExtension);
        }
    }

    SECTION("Request unavailable extension does not fail")
    {
        const auto result = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                                .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                                .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                                .RequestExtension("VK_FAKE_does_not_exist")
                                .Select();

        // Should succeed - requested is optional
        if (result)
        {
            VKit::PhysicalDevice device = result.GetValue();
            CHECK_FALSE(device.IsExtensionEnabled("VK_FAKE_does_not_exist"));
        }
    }
}

TEST_CASE("PhysicalDevice::Selector - Memory Requirements", "[physical_device][selector][memory]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    auto instanceResult = CreateValidatedInstance();
    REQUIRE(instanceResult);
    InstanceGuard instanceGuard(std::move(instanceResult.GetValue()));

    SECTION("Require reasonable memory amount")
    {
        const auto result = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                                .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                                .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                                .RequireMemory(64 * 1024 * 1024) // 64 MB - should be available
                                .Select();

        if (result)
        {
            VKit::PhysicalDevice device = result.GetValue();
            // Verify device has device-local memory
            const auto &memProps = device.GetInfo().Properties.Memory;
            bool hasDeviceLocal = false;
            for (u32 i = 0; i < memProps.memoryHeapCount; ++i)
            {
                if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                {
                    hasDeviceLocal = true;
                    break;
                }
            }
            CHECK(hasDeviceLocal);
        }
    }

    SECTION("Require impossibly large memory fails")
    {
        const auto result = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                                .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                                .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                                .RequireMemory(1024ULL * 1024 * 1024 * 1024) // 1 TB
                                .Select();

        CHECK_FALSE(result);
        if (!result)
        {
            CHECK(result.GetError().GetCode() == VKit::Error_InsufficientMemory);
        }
    }
}

TEST_CASE("PhysicalDevice::Selector - API Version", "[physical_device][selector][version]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    auto instanceResult = CreateValidatedInstance();
    REQUIRE(instanceResult);
    InstanceGuard instanceGuard(std::move(instanceResult.GetValue()));

    SECTION("Require API version 1.0.0 succeeds")
    {
        const auto result = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                                .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                                .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                                .RequireApiVersion(1, 0, 0)
                                .Select();

        if (result)
        {
            VKit::PhysicalDevice device = result.GetValue();
            CHECK(device.GetInfo().ApiVersion >= VKIT_MAKE_VERSION(0, 1, 0, 0));
        }
    }

    SECTION("Require impossibly high API version fails")
    {
        const auto result = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                                .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                                .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                                .RequireApiVersion(99, 0, 0)
                                .Select();

        CHECK_FALSE(result);
        if (!result)
        {
            CHECK(result.GetError().GetCode() == VKit::Error_VersionMismatch);
        }
    }
}

TEST_CASE("PhysicalDevice - Feature Queries", "[physical_device][features]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    auto instanceResult = CreateValidatedInstance();
    REQUIRE(instanceResult);
    InstanceGuard instanceGuard(std::move(instanceResult.GetValue()));

    auto deviceResult = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                            .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                            .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                            .Select();

    if (!deviceResult)
    {
        SKIP("No physical device available");
    }

    VKit::PhysicalDevice device = deviceResult.GetValue();

    SECTION("Can query available features")
    {
        const auto &features = device.GetInfo().AvailableFeatures;

        // Just verify we can access the features
        [[maybe_unused]] VkBool32 robustBuffer = features.Core.robustBufferAccess;
        [[maybe_unused]] VkBool32 geometryShader = features.Core.geometryShader;
        [[maybe_unused]] VkBool32 tessellation = features.Core.tessellationShader;
    }

    SECTION("AreFeaturesSupported returns true for empty features")
    {
        VKit::DeviceFeatures emptyFeatures{};
        CHECK(device.AreFeaturesSupported(emptyFeatures));
    }

    SECTION("EnableFeatures with supported features succeeds")
    {
        VKit::DeviceFeatures features{};
        // Most devices support robustBufferAccess
        if (device.GetInfo().AvailableFeatures.Core.robustBufferAccess)
        {
            features.Core.robustBufferAccess = VK_TRUE;
            CHECK(device.EnableFeatures(features));
            CHECK(device.GetInfo().EnabledFeatures.Core.robustBufferAccess == VK_TRUE);
        }
    }
}

TEST_CASE("PhysicalDevice - Extension Management", "[physical_device][extensions]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    auto instanceResult = CreateValidatedInstance();
    REQUIRE(instanceResult);
    InstanceGuard instanceGuard(std::move(instanceResult.GetValue()));

    auto deviceResult = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                            .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                            .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                            .Select();

    if (!deviceResult)
    {
        SKIP("No physical device available");
    }

    VKit::PhysicalDevice device = deviceResult.GetValue();

    SECTION("Can query available extensions")
    {
        const auto &extensions = device.GetInfo().AvailableExtensions;
        INFO("Device has " << extensions.GetSize() << " available extensions");
        CHECK_FALSE(extensions.IsEmpty());
    }

    SECTION("IsExtensionSupported returns false for fake extension")
    {
        CHECK_FALSE(device.IsExtensionSupported("VK_FAKE_does_not_exist"));
    }

    SECTION("EnableExtension with supported extension succeeds")
    {
        // Find any supported extension
        const auto &extensions = device.GetInfo().AvailableExtensions;
        if (!extensions.IsEmpty())
        {
            const char *extName = extensions[0].c_str();
            CHECK(device.EnableExtension(extName));
            CHECK(device.IsExtensionEnabled(extName));
        }
    }

    SECTION("EnableExtension with unsupported extension fails")
    {
        CHECK_FALSE(device.EnableExtension("VK_FAKE_does_not_exist"));
    }
}

// ============================================================================
// LOGICAL DEVICE BUILDER TESTS
// ============================================================================

TEST_CASE("LogicalDevice::Builder - Basic Creation", "[logical_device][builder][create]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    auto instanceResult = CreateValidatedInstance();
    REQUIRE(instanceResult);
    InstanceGuard instanceGuard(std::move(instanceResult.GetValue()));

    auto physicalResult = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                              .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                              .AddFlags(VKit::DeviceSelectorFlag_RequireGraphicsQueue)
                              .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                              .Select();

    if (!physicalResult)
    {
        SKIP("No suitable physical device available");
    }

    VKit::PhysicalDevice physicalDevice = physicalResult.GetValue();

    SECTION("Create device with single graphics queue")
    {
        const auto result = VKit::LogicalDevice::Builder(&instanceGuard.Get(), &physicalDevice)
                                .RequireQueue(VKit::Queue_Graphics, 1)
                                .Build();

        REQUIRE(result);

        VKit::LogicalDevice device = result.GetValue();
        CHECK(device.GetHandle() != VK_NULL_HANDLE);

        const auto &queues = device.GetInfo().QueuesPerType[VKit::Queue_Graphics];
        CHECK_FALSE(queues.IsEmpty());

        device.Destroy();
    }

    SECTION("Create device with multiple queue types")
    {
        auto result = VKit::LogicalDevice::Builder(&instanceGuard.Get(), &physicalDevice)
                          .RequireQueue(VKit::Queue_Graphics, 1, 1.0f)
                          .RequestQueue(VKit::Queue_Compute, 1, 0.5f)
                          .RequestQueue(VKit::Queue_Transfer, 1, 0.25f)
                          .Build();

        REQUIRE(result);

        VKit::LogicalDevice device = result.GetValue();
        CHECK(device.GetHandle() != VK_NULL_HANDLE);

        // Graphics should definitely exist
        CHECK_FALSE(device.GetInfo().QueuesPerType[VKit::Queue_Graphics].IsEmpty());

        device.Destroy();
    }

    SECTION("Create device with multiple queues of same type")
    {
        auto result = VKit::LogicalDevice::Builder(&instanceGuard.Get(), &physicalDevice)
                          .RequireQueue(VKit::Queue_Graphics, 2, 1.0f) // Request 2 graphics queues
                          .Build();

        // May succeed or fail depending on queue count support
        if (result)
        {
            VKit::LogicalDevice device = result.GetValue();
            const auto &queues = device.GetInfo().QueuesPerType[VKit::Queue_Graphics];
            INFO("Created " << queues.GetSize() << " graphics queue(s)");
            device.Destroy();
        }
    }
}

TEST_CASE("LogicalDevice::Builder - Queue Priority", "[logical_device][builder][priority]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    auto instanceResult = CreateValidatedInstance();
    REQUIRE(instanceResult);
    InstanceGuard instanceGuard(std::move(instanceResult.GetValue()));

    auto physicalResult = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                              .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                              .AddFlags(VKit::DeviceSelectorFlag_RequireGraphicsQueue)
                              .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                              .Select();

    if (!physicalResult)
    {
        SKIP("No suitable physical device available");
    }

    VKit::PhysicalDevice physicalDevice = physicalResult.GetValue();

    SECTION("Different priorities for queues")
    {
        auto result = VKit::LogicalDevice::Builder(&instanceGuard.Get(), &physicalDevice)
                          .RequireQueue(VKit::Queue_Graphics, 1, 1.0f)  // High priority
                          .RequestQueue(VKit::Queue_Compute, 1, 0.5f)   // Medium priority
                          .RequestQueue(VKit::Queue_Transfer, 1, 0.25f) // Low priority
                          .Build();

        REQUIRE(result);

        VKit::LogicalDevice device = result.GetValue();
        CHECK(device.GetHandle() != VK_NULL_HANDLE);

        device.Destroy();
    }
}

TEST_CASE("LogicalDevice - Destruction", "[logical_device][destroy]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    auto instanceResult = CreateValidatedInstance();
    REQUIRE(instanceResult);
    InstanceGuard instanceGuard(std::move(instanceResult.GetValue()));

    auto physicalResult = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                              .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                              .AddFlags(VKit::DeviceSelectorFlag_RequireGraphicsQueue)
                              .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                              .Select();

    if (!physicalResult)
    {
        SKIP("No suitable physical device available");
    }

    VKit::PhysicalDevice physicalDevice = physicalResult.GetValue();

    SECTION("Destroy sets handle to null")
    {
        auto result = VKit::LogicalDevice::Builder(&instanceGuard.Get(), &physicalDevice)
                          .RequireQueue(VKit::Queue_Graphics, 1)
                          .Build();

        REQUIRE(result);

        VKit::LogicalDevice device = result.GetValue();
        CHECK(device.GetHandle() != VK_NULL_HANDLE);

        device.Destroy();
        CHECK(device.GetHandle() == VK_NULL_HANDLE);
    }

    SECTION("Double destroy is safe")
    {
        auto result = VKit::LogicalDevice::Builder(&instanceGuard.Get(), &physicalDevice)
                          .RequireQueue(VKit::Queue_Graphics, 1)
                          .Build();

        REQUIRE(result);

        VKit::LogicalDevice device = result.GetValue();
        device.Destroy();
        device.Destroy(); // Should not crash

        CHECK(device.GetHandle() == VK_NULL_HANDLE);
    }
}

TEST_CASE("LogicalDevice - WaitIdle", "[logical_device][wait]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    auto instanceResult = CreateValidatedInstance();
    REQUIRE(instanceResult);
    InstanceGuard instanceGuard(std::move(instanceResult.GetValue()));

    auto physicalResult = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                              .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                              .AddFlags(VKit::DeviceSelectorFlag_RequireGraphicsQueue)
                              .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                              .Select();

    if (!physicalResult)
    {
        SKIP("No suitable physical device available");
    }

    VKit::PhysicalDevice physicalDevice = physicalResult.GetValue();

    auto deviceResult = VKit::LogicalDevice::Builder(&instanceGuard.Get(), &physicalDevice)
                            .RequireQueue(VKit::Queue_Graphics, 1)
                            .Build();

    REQUIRE(deviceResult);
    VKit::LogicalDevice device = deviceResult.GetValue();

    SECTION("WaitIdle on idle device succeeds")
    {
        auto result = device.WaitIdle();
        REQUIRE(result);
    }

    SECTION("Multiple WaitIdle calls succeed")
    {
        for (int i = 0; i < 5; ++i)
        {
            auto result = device.WaitIdle();
            REQUIRE(result);
        }
    }

    SECTION("Static WaitIdle with proxy succeeds")
    {
        auto proxy = device.CreateProxy();
        auto result = VKit::LogicalDevice::WaitIdle(proxy);
        REQUIRE(result);
    }

    device.Destroy();
}
TEST_CASE("LogicalDevice - Proxy Creation", "[logical_device][proxy]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    auto instanceResult = CreateValidatedInstance();
    REQUIRE(instanceResult);
    InstanceGuard instanceGuard(std::move(instanceResult.GetValue()));

    auto physicalResult = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                              .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                              .AddFlags(VKit::DeviceSelectorFlag_RequireGraphicsQueue)
                              .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                              .Select();

    if (!physicalResult)
    {
        SKIP("No suitable physical device available");
    }

    VKit::PhysicalDevice physicalDevice = physicalResult.GetValue();

    auto deviceResult = VKit::LogicalDevice::Builder(&instanceGuard.Get(), &physicalDevice)
                            .RequireQueue(VKit::Queue_Graphics, 1)
                            .Build();

    REQUIRE(deviceResult);
    VKit::LogicalDevice device = deviceResult.GetValue();

    SECTION("CreateProxy returns valid proxy")
    {
        auto proxy = device.CreateProxy();

        CHECK(proxy.Device == device.GetHandle());
        CHECK(proxy.Table != nullptr);
    }

    device.Destroy();
}

TEST_CASE("LogicalDevice - FindSupportedFormat", "[logical_device][format]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    auto instanceResult = CreateValidatedInstance();
    REQUIRE(instanceResult);
    InstanceGuard instanceGuard(std::move(instanceResult.GetValue()));

    auto physicalResult = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                              .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                              .AddFlags(VKit::DeviceSelectorFlag_RequireGraphicsQueue)
                              .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                              .Select();

    if (!physicalResult)
    {
        SKIP("No suitable physical device available");
    }

    VKit::PhysicalDevice physicalDevice = physicalResult.GetValue();

    auto deviceResult = VKit::LogicalDevice::Builder(&instanceGuard.Get(), &physicalDevice)
                            .RequireQueue(VKit::Queue_Graphics, 1)
                            .Build();

    REQUIRE(deviceResult);
    VKit::LogicalDevice device = deviceResult.GetValue();

    SECTION("Find depth format")
    {
        TKit::FixedArray<VkFormat, 3> candidates = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                                                    VK_FORMAT_D24_UNORM_S8_UINT};

        auto result =
            device.FindSupportedFormat(TKit::Span<const VkFormat>(candidates.GetData(), candidates.GetSize()),
                                       VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

        if (result)
        {
            VkFormat format = result.GetValue();
            CHECK((format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                   format == VK_FORMAT_D24_UNORM_S8_UINT));
        }
    }

    SECTION("Find color format for sampling")
    {
        TKit::FixedArray<VkFormat, 2> candidates = {VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB};

        auto result = device.FindSupportedFormat(TKit::Span<const VkFormat>(candidates.GetData(), candidates.GetSize()),
                                                 VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

        if (result)
        {
            VkFormat format = result.GetValue();
            CHECK((format == VK_FORMAT_R8G8B8A8_SRGB || format == VK_FORMAT_B8G8R8A8_SRGB));
        }
    }

    SECTION("No supported format returns error")
    {
        TKit::FixedArray<VkFormat, 1> candidates = {VK_FORMAT_UNDEFINED};

        auto result = device.FindSupportedFormat(TKit::Span<const VkFormat>(candidates.GetData(), candidates.GetSize()),
                                                 VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

        CHECK_FALSE(result);
        if (!result)
        {
            CHECK(result.GetError().GetCode() == VKit::Error_NoFormatSupported);
        }
    }

    device.Destroy();
}

// ============================================================================
// QUEUE ACCESS FROM LOGICAL DEVICE
// ============================================================================

TEST_CASE("LogicalDevice - Queue Access", "[logical_device][queues]")
{
    CoreGuard coreGuard;
    REQUIRE(coreGuard.IsValid());

    auto instanceResult = CreateValidatedInstance();
    REQUIRE(instanceResult);
    InstanceGuard instanceGuard(std::move(instanceResult.GetValue()));

    auto physicalResult = VKit::PhysicalDevice::Selector(&instanceGuard.Get())
                              .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                              .AddFlags(VKit::DeviceSelectorFlag_RequireGraphicsQueue)
                              .AddFlags(VKit::DeviceSelectorFlag_RequireComputeQueue)
                              .AddFlags(VKit::DeviceSelectorFlag_RequireTransferQueue)
                              .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                              .Select();

    if (!physicalResult)
    {
        SKIP("No suitable physical device with all queue types available");
    }

    VKit::PhysicalDevice physicalDevice = physicalResult.GetValue();

    auto deviceResult = VKit::LogicalDevice::Builder(&instanceGuard.Get(), &physicalDevice)
                            .RequireQueue(VKit::Queue_Graphics, 1, 1.0f)
                            .RequestQueue(VKit::Queue_Compute, 1, 0.5f)
                            .RequestQueue(VKit::Queue_Transfer, 1, 0.25f)
                            .Build();

    REQUIRE(deviceResult);
    VKit::LogicalDevice device = deviceResult.GetValue();

    SECTION("Can access graphics queue")
    {
        const auto &queues = device.GetInfo().QueuesPerType[VKit::Queue_Graphics];
        REQUIRE_FALSE(queues.IsEmpty());

        VKit::Queue *queue = queues[0];
        CHECK(queue != nullptr);
        CHECK(queue->GetHandle() != VK_NULL_HANDLE);
    }

    SECTION("Queue family indices are valid")
    {
        const auto &queues = device.GetInfo().QueuesPerType[VKit::Queue_Graphics];
        if (!queues.IsEmpty())
        {
            VKit::Queue *queue = queues[0];
            u32 family = queue->GetFamily();
            CHECK(family == physicalDevice.GetInfo().FamilyIndices[VKit::Queue_Graphics]);
        }
    }

    SECTION("Queue operations work")
    {
        const auto &queues = device.GetInfo().QueuesPerType[VKit::Queue_Graphics];
        REQUIRE_FALSE(queues.IsEmpty());

        VKit::Queue *queue = queues[0];
        auto result = queue->WaitIdle();
        REQUIRE(result);
    }

    device.Destroy();
}

// ============================================================================
// FULL PIPELINE INTEGRATION TEST
// ============================================================================

TEST_CASE("Full initialization pipeline", "[integration][pipeline]")
{
    // Step 1: Initialize Core
    auto coreResult = VKit::Core::Initialize();

    {
        REQUIRE(coreResult);

        // Step 2: Create Instance
        auto instanceResult = VKit::Instance::Builder()
                                  .SetApplicationName("Full Pipeline Test")
                                  .SetApplicationVersion(1, 0, 0)
                                  .SetEngineName("Test Engine")
                                  .SetEngineVersion(1, 0, 0)
                                  .RequireApiVersion(1, 0, 0)
                                  .RequestApiVersion(1, 2, 0)
                                  .RequireLayer("VK_LAYER_KHRONOS_validation")
                                  .SetHeadless(true)
                                  .Build();

        REQUIRE(instanceResult);
        VKit::Instance instance = instanceResult.GetValue();

        // Step 3: Select Physical Device
        auto physicalResult = VKit::PhysicalDevice::Selector(&instance)
                                  .PreferType(VKit::Device_Discrete)
                                  .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                                  .AddFlags(VKit::DeviceSelectorFlag_RequireGraphicsQueue)
                                  .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                                  .Select();

        if (!physicalResult)
        {
            instance.Destroy();
            VKit::Core::Terminate();
            SKIP("No physical device available");
        }

        VKit::PhysicalDevice physicalDevice = physicalResult.GetValue();

        // Log device info
        INFO("Selected device: " << physicalDevice.GetInfo().Properties.Core.deviceName);
        INFO("API Version: " << VK_VERSION_MAJOR(physicalDevice.GetInfo().ApiVersion) << "."
                             << VK_VERSION_MINOR(physicalDevice.GetInfo().ApiVersion) << "."
                             << VK_VERSION_PATCH(physicalDevice.GetInfo().ApiVersion));

        // Step 4: Create Logical Device
        auto deviceResult = VKit::LogicalDevice::Builder(&instance, &physicalDevice)
                                .RequireQueue(VKit::Queue_Graphics, 1, 1.0f)
                                .RequestQueue(VKit::Queue_Compute, 1, 0.5f)
                                .Build();

        REQUIRE(deviceResult);
        VKit::LogicalDevice device = deviceResult.GetValue();

        // Verify everything is set up correctly
        CHECK(device.GetHandle() != VK_NULL_HANDLE);
        CHECK_FALSE(device.GetInfo().QueuesPerType[VKit::Queue_Graphics].IsEmpty());

        // Can perform basic operations
        auto waitResult = device.WaitIdle();
        REQUIRE(waitResult);

        // Step 5: Clean teardown (reverse order)
        device.Destroy();
        instance.Destroy();
        // Verify cleanup
        CHECK(device.GetHandle() == VK_NULL_HANDLE);
        CHECK(instance.GetHandle() == VK_NULL_HANDLE);
    }
    VKit::Core::Terminate();
}
