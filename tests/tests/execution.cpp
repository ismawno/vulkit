/**
 * @file test_execution.cpp
 * @brief Focused Catch2 test suite for VKit CommandPool and Queue
 *
 * This test file provides comprehensive coverage of the execution components
 * with minimal dependencies. Designed to integrate easily into existing builds.
 */

#undef VKIT_NO_DISCARD
#define VKIT_NO_DISCARD

#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_section_info.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include "vkit/core/core.hpp"
#include "vkit/vulkan/instance.hpp"
#include "vkit/device/physical_device.hpp"
#include "vkit/device/logical_device.hpp"
#include "vkit/execution/command_pool.hpp"
#include "vkit/execution/queue.hpp"

#include <vector>

using namespace TKit::Alias;

namespace
{

// ============================================================================
// Test Context Management
// ============================================================================

/**
 * @brief Singleton test context for Vulkan resources
 *
 * Ensures single initialization across all test cases and proper cleanup.
 */
class TestContext
{
  public:
    static TestContext &Get()
    {
        static TestContext instance;
        return instance;
    }

    bool IsValid() const
    {
        return m_Valid;
    }

    VKit::ProxyDevice GetProxy() const
    {
        return m_LogicalDevice->CreateProxy();
    }

    const VKit::Instance &GetInstance() const
    {
        return *m_Instance;
    }

    const VKit::PhysicalDevice &GetPhysicalDevice() const
    {
        return *m_PhysicalDevice;
    }

    const VKit::LogicalDevice &GetLogicalDevice() const
    {
        return *m_LogicalDevice;
    }

    u32 GetGraphicsFamily() const
    {
        return m_PhysicalDevice->GetInfo().FamilyIndices[VKit::Queue_Graphics];
    }

    u32 GetComputeFamily() const
    {
        return m_PhysicalDevice->GetInfo().FamilyIndices[VKit::Queue_Compute];
    }

    u32 GetTransferFamily() const
    {
        return m_PhysicalDevice->GetInfo().FamilyIndices[VKit::Queue_Transfer];
    }

    VKit::Queue *GetGraphicsQueue()
    {
        const auto &queues = m_LogicalDevice->GetInfo().QueuesPerType[VKit::Queue_Graphics];
        return queues.IsEmpty() ? nullptr : queues[0];
    }

    VKit::Queue *GetComputeQueue()
    {
        const auto &queues = m_LogicalDevice->GetInfo().QueuesPerType[VKit::Queue_Compute];
        return queues.IsEmpty() ? nullptr : queues[0];
    }

    VKit::Queue *GetTransferQueue()
    {
        const auto &queues = m_LogicalDevice->GetInfo().QueuesPerType[VKit::Queue_Transfer];
        return queues.IsEmpty() ? nullptr : queues[0];
    }

    void WaitIdle()
    {
        if (m_Valid)
            m_LogicalDevice->WaitIdle();
    }

  private:
    TestContext()
    {
        Initialize();
    }

    ~TestContext()
    {
        Shutdown();
    }

    void Initialize()
    {
        // Initialize Vulkan Core
        auto coreResult = VKit::Core::Initialize();
        if (!coreResult)
        {
            m_ErrorMessage = "Failed to initialize Core: " + std::string(coreResult.GetError().GetMessage());
            return;
        }

        // Create headless instance
        auto instanceResult = VKit::Instance::Builder()
                                  .SetApplicationName("VKit Execution Tests")
                                  .SetApplicationVersion(1, 0, 0)
                                  .SetEngineName("VKit Test Engine")
                                  .RequireApiVersion(1, 0, 0)
                                  .RequestApiVersion(1, 2, 0)
                                  .RequestValidationLayers()
                                  .SetHeadless(true)
                                  .Build();

        if (!instanceResult)
        {
            m_ErrorMessage = "Failed to create Instance: " + std::string(instanceResult.GetError().GetMessage());
            VKit::Core::Terminate();
            return;
        }
        m_Instance = new VKit::Instance(instanceResult.GetValue());

        // Select physical device with all queue types
        auto physicalResult = VKit::PhysicalDevice::Selector(m_Instance)
                                  .PreferType(VKit::Device_Discrete)
                                  .AddFlags(VKit::DeviceSelectorFlag_AnyType)
                                  .AddFlags(VKit::DeviceSelectorFlag_RequireGraphicsQueue)
                                  .AddFlags(VKit::DeviceSelectorFlag_RequireComputeQueue)
                                  .AddFlags(VKit::DeviceSelectorFlag_RequireTransferQueue)
                                  .RemoveFlags(VKit::DeviceSelectorFlag_RequirePresentQueue)
                                  .Select();

        if (!physicalResult)
        {
            m_ErrorMessage = "Failed to select Physical Device: " + std::string(physicalResult.GetError().GetMessage());
            m_Instance->Destroy();
            delete m_Instance;
            m_Instance = nullptr;
            VKit::Core::Terminate();
            return;
        }
        m_PhysicalDevice = new VKit::PhysicalDevice(physicalResult.GetValue());

        // Create logical device with multiple queue types
        auto logicalResult = VKit::LogicalDevice::Builder(m_Instance, m_PhysicalDevice)
                                 .RequireQueue(VKit::Queue_Graphics, 1, 1.0f)
                                 .RequestQueue(VKit::Queue_Compute, 1, 0.8f)
                                 .RequestQueue(VKit::Queue_Transfer, 1, 0.5f)
                                 .Build();

        if (!logicalResult)
        {
            m_ErrorMessage = "Failed to create Logical Device: " + std::string(logicalResult.GetError().GetMessage());
            m_Instance->Destroy();
            delete m_PhysicalDevice;
            delete m_Instance;
            m_PhysicalDevice = nullptr;
            m_Instance = nullptr;
            VKit::Core::Terminate();
            return;
        }
        m_LogicalDevice = new VKit::LogicalDevice(logicalResult.GetValue());
        m_Valid = true;
    }

    void Shutdown()
    {
        if (m_Valid)
        {
            m_LogicalDevice->WaitIdle();
            m_LogicalDevice->Destroy();
            m_Instance->Destroy();
            delete m_LogicalDevice;
            delete m_PhysicalDevice;
            delete m_Instance;
            VKit::Core::Terminate();
            m_Valid = false;
        }
    }

    bool m_Valid = false;
    std::string m_ErrorMessage;
    VKit::Instance *m_Instance = nullptr;
    VKit::PhysicalDevice *m_PhysicalDevice = nullptr;
    VKit::LogicalDevice *m_LogicalDevice = nullptr;
};

/**
 * @brief RAII guard ensuring test context is valid
 */
struct ContextGuard
{
    ContextGuard()
    {
        REQUIRE(TestContext::Get().IsValid());
    }
};

} // anonymous namespace

// ============================================================================
// COMMAND POOL - CREATION TESTS
// ============================================================================

TEST_CASE("CommandPool::Create - Basic Creation", "[command_pool][create]")
{
    ContextGuard guard;
    auto &ctx = TestContext::Get();
    auto proxy = ctx.GetProxy();

    SECTION("Creates pool with zero flags for graphics queue")
    {
        auto result = VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), 0);

        REQUIRE(result);
        auto pool = result.GetValue();
        CHECK(pool.GetHandle() != VK_NULL_HANDLE);

        pool.Destroy();
    }

    SECTION("Creates pool with RESET_COMMAND_BUFFER_BIT flag")
    {
        auto result =
            VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        REQUIRE(result);
        auto pool = result.GetValue();
        CHECK(pool.GetHandle() != VK_NULL_HANDLE);

        pool.Destroy();
    }

    SECTION("Creates pool with TRANSIENT_BIT flag")
    {
        auto result = VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

        REQUIRE(result);
        auto pool = result.GetValue();
        CHECK(pool.GetHandle() != VK_NULL_HANDLE);

        pool.Destroy();
    }

    SECTION("Creates pool with PROTECTED_BIT flag if supported")
    {
        // Note: VK_COMMAND_POOL_CREATE_PROTECTED_BIT requires protected memory feature
        // Skip if not supported
        const auto &features = ctx.GetPhysicalDevice().GetInfo().AvailableFeatures;
#ifdef VKIT_API_VERSION_1_2
        if (features.Vulkan11.protectedMemory)
        {
            auto result =
                VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), VK_COMMAND_POOL_CREATE_PROTECTED_BIT);
            // May or may not succeed depending on device support
            if (result)
            {
                auto pool = result.GetValue();
                pool.Destroy();
            }
        }
#endif
    }

    SECTION("Creates pools for different queue families")
    {
        auto graphicsResult = VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), 0);
        auto computeResult = VKit::CommandPool::Create(proxy, ctx.GetComputeFamily(), 0);
        auto transferResult = VKit::CommandPool::Create(proxy, ctx.GetTransferFamily(), 0);

        REQUIRE(graphicsResult);
        REQUIRE(computeResult);
        REQUIRE(transferResult);

        auto graphicsPool = graphicsResult.GetValue();
        auto computePool = computeResult.GetValue();
        auto transferPool = transferResult.GetValue();

        // Handles should be unique
        CHECK(graphicsPool.GetHandle() != computePool.GetHandle());
        CHECK(computePool.GetHandle() != transferPool.GetHandle());
        CHECK(graphicsPool.GetHandle() != transferPool.GetHandle());

        graphicsPool.Destroy();
        computePool.Destroy();
        transferPool.Destroy();
    }
}

// ============================================================================
// COMMAND POOL - ALLOCATION TESTS
// ============================================================================

TEST_CASE("CommandPool::Allocate - Single Buffer", "[command_pool][allocate]")
{
    ContextGuard guard;
    auto &ctx = TestContext::Get();
    auto proxy = ctx.GetProxy();

    auto poolResult = VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), 0);
    REQUIRE(poolResult);
    auto pool = poolResult.GetValue();

    SECTION("Allocates primary command buffer")
    {
        auto result = pool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

        REQUIRE(result);
        CHECK(result.GetValue() != VK_NULL_HANDLE);

        pool.Deallocate(result.GetValue());
    }

    SECTION("Allocates secondary command buffer")
    {
        auto result = pool.Allocate(VK_COMMAND_BUFFER_LEVEL_SECONDARY);

        REQUIRE(result);
        CHECK(result.GetValue() != VK_NULL_HANDLE);

        pool.Deallocate(result.GetValue());
    }

    SECTION("Multiple sequential allocations return unique handles")
    {
        std::vector<VkCommandBuffer> buffers;
        constexpr int count = 10;

        for (int i = 0; i < count; ++i)
        {
            auto result = pool.Allocate();
            REQUIRE(result);
            buffers.push_back(result.GetValue());
        }

        // Verify all handles are unique
        for (size_t i = 0; i < buffers.size(); ++i)
        {
            for (size_t j = i + 1; j < buffers.size(); ++j)
            {
                CHECK(buffers[i] != buffers[j]);
            }
        }

        for (auto buf : buffers)
        {
            pool.Deallocate(buf);
        }
    }

    pool.Destroy();
}

TEST_CASE("CommandPool::Allocate - Batch Allocation", "[command_pool][allocate][batch]")
{
    ContextGuard guard;
    auto &ctx = TestContext::Get();
    auto proxy = ctx.GetProxy();

    auto poolResult = VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), 0);
    REQUIRE(poolResult);
    auto pool = poolResult.GetValue();

    SECTION("Allocates batch of primary buffers")
    {
        constexpr u32 batchSize = 8;
        TKit::FixedArray<VkCommandBuffer, batchSize> buffers;

        auto result = pool.Allocate(TKit::Span<VkCommandBuffer>(buffers.GetData(), batchSize));

        REQUIRE(result);

        for (u32 i = 0; i < batchSize; ++i)
        {
            CHECK(buffers[i] != VK_NULL_HANDLE);
        }

        pool.Deallocate(TKit::Span<const VkCommandBuffer>(buffers.GetData(), batchSize));
    }

    SECTION("Allocates batch of secondary buffers")
    {
        constexpr u32 batchSize = 4;
        TKit::FixedArray<VkCommandBuffer, batchSize> buffers;

        auto result =
            pool.Allocate(TKit::Span<VkCommandBuffer>(buffers.GetData(), batchSize), VK_COMMAND_BUFFER_LEVEL_SECONDARY);

        REQUIRE(result);

        for (u32 i = 0; i < batchSize; ++i)
        {
            CHECK(buffers[i] != VK_NULL_HANDLE);
        }

        pool.Deallocate(TKit::Span<const VkCommandBuffer>(buffers.GetData(), batchSize));
    }

    SECTION("Large batch allocation")
    {
        constexpr u32 batchSize = 64;
        TKit::FixedArray64<VkCommandBuffer> buffers;

        auto result = pool.Allocate(TKit::Span<VkCommandBuffer>(buffers.GetData(), batchSize));

        REQUIRE(result);

        // Verify uniqueness
        for (u32 i = 0; i < batchSize; ++i)
        {
            CHECK(buffers[i] != VK_NULL_HANDLE);
            for (u32 j = i + 1; j < batchSize; ++j)
            {
                CHECK(buffers[i] != buffers[j]);
            }
        }

        pool.Deallocate(TKit::Span<const VkCommandBuffer>(buffers.GetData(), batchSize));
    }

    pool.Destroy();
}

// ============================================================================
// COMMAND POOL - RESET TESTS
// ============================================================================

TEST_CASE("CommandPool::Reset", "[command_pool][reset]")
{
    ContextGuard guard;
    auto &ctx = TestContext::Get();
    auto proxy = ctx.GetProxy();

    SECTION("Reset empty pool succeeds")
    {
        auto poolResult = VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), 0);
        REQUIRE(poolResult);
        auto pool = poolResult.GetValue();

        auto result = pool.Reset();
        REQUIRE(result);

        pool.Destroy();
    }

    SECTION("Reset pool with allocated buffers")
    {
        auto poolResult = VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), 0);
        REQUIRE(poolResult);
        auto pool = poolResult.GetValue();

        // Allocate some buffers
        constexpr u32 count = 5;
        TKit::FixedArray<VkCommandBuffer, count> buffers;

        auto allocResult = pool.Allocate(TKit::Span<VkCommandBuffer>(buffers.GetData(), count));
        REQUIRE(allocResult);

        auto resetResult = pool.Reset();
        REQUIRE(resetResult);

        pool.Destroy();
    }

    SECTION("Reset with RELEASE_RESOURCES flag")
    {
        auto poolResult = VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), 0);
        REQUIRE(poolResult);
        auto pool = poolResult.GetValue();

        auto allocResult = pool.Allocate();
        REQUIRE(allocResult);

        auto resetResult = pool.Reset(VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
        REQUIRE(resetResult);

        pool.Destroy();
    }

    SECTION("Multiple consecutive resets")
    {
        auto poolResult = VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), 0);
        REQUIRE(poolResult);
        auto pool = poolResult.GetValue();

        for (int i = 0; i < 10; ++i)
        {
            auto result = pool.Reset();
            REQUIRE(result);
        }

        pool.Destroy();
    }

    SECTION("Allocate after reset")
    {
        auto poolResult =
            VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        REQUIRE(poolResult);
        auto pool = poolResult.GetValue();

        // First allocation
        auto alloc1 = pool.Allocate();
        REQUIRE(alloc1);

        // Reset
        auto resetResult = pool.Reset();
        REQUIRE(resetResult);

        // Allocate again after reset
        auto alloc2 = pool.Allocate();
        REQUIRE(alloc2);
        CHECK(alloc2.GetValue() != VK_NULL_HANDLE);

        pool.Destroy();
    }
}

// ============================================================================
// COMMAND POOL - SINGLE TIME COMMANDS TESTS
// ============================================================================

TEST_CASE("CommandPool::BeginSingleTimeCommands", "[command_pool][single_time]")
{
    ContextGuard guard;
    auto &ctx = TestContext::Get();
    auto proxy = ctx.GetProxy();
    auto *queue = ctx.GetGraphicsQueue();
    REQUIRE(queue != nullptr);

    auto poolResult =
        VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    REQUIRE(poolResult);
    auto pool = poolResult.GetValue();

    SECTION("Begin returns valid command buffer in recording state")
    {
        auto result = pool.BeginSingleTimeCommands();

        REQUIRE(result);
        CHECK(result.GetValue() != VK_NULL_HANDLE);

        // End to clean up
        auto endResult = pool.EndSingleTimeCommands(result.GetValue(), *queue);
        REQUIRE(endResult);
    }

    SECTION("Complete workflow: begin -> record -> end")
    {
        auto beginResult = pool.BeginSingleTimeCommands();
        REQUIRE(beginResult);

        VkCommandBuffer cmd = beginResult.GetValue();

        // Record a simple command (memory barrier)
        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        proxy.Table->CmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 1,
                                        &barrier, 0, nullptr, 0, nullptr);

        auto endResult = pool.EndSingleTimeCommands(cmd, *queue);
        REQUIRE(endResult);
    }

    SECTION("Multiple consecutive single time command operations")
    {
        constexpr int iterations = 10;

        for (int i = 0; i < iterations; ++i)
        {
            auto beginResult = pool.BeginSingleTimeCommands();
            REQUIRE(beginResult);

            auto endResult = pool.EndSingleTimeCommands(beginResult.GetValue(), *queue);
            REQUIRE(endResult);
        }
    }

    pool.Destroy();
}

// ============================================================================
// COMMAND POOL - DESTRUCTION TESTS
// ============================================================================

TEST_CASE("CommandPool::Destroy", "[command_pool][destroy]")
{
    ContextGuard guard;
    auto &ctx = TestContext::Get();
    auto proxy = ctx.GetProxy();

    SECTION("Destroy sets handle to null")
    {
        auto poolResult = VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), 0);
        REQUIRE(poolResult);
        auto pool = poolResult.GetValue();

        CHECK(pool.GetHandle() != VK_NULL_HANDLE);

        pool.Destroy();

        CHECK(pool.GetHandle() == VK_NULL_HANDLE);
    }

    SECTION("Double destroy is safe")
    {
        auto poolResult = VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), 0);
        REQUIRE(poolResult);
        auto pool = poolResult.GetValue();

        pool.Destroy();
        pool.Destroy(); // Should not crash

        CHECK(pool.GetHandle() == VK_NULL_HANDLE);
    }

    SECTION("Destroy pool with outstanding allocations")
    {
        auto poolResult = VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), 0);
        REQUIRE(poolResult);
        auto pool = poolResult.GetValue();

        auto allocResult = pool.Allocate();
        REQUIRE(allocResult);

        // Destroy without explicitly freeing - Vulkan spec allows this
        ctx.WaitIdle();
        pool.Destroy();

        CHECK(pool.GetHandle() == VK_NULL_HANDLE);
    }
}

// ============================================================================
// QUEUE - BASIC TESTS
// ============================================================================

TEST_CASE("Queue - Basic Properties", "[queue][properties]")
{
    ContextGuard guard;
    auto &ctx = TestContext::Get();

    SECTION("Graphics queue has valid handle and family")
    {
        auto *queue = ctx.GetGraphicsQueue();
        REQUIRE(queue != nullptr);

        CHECK(queue->GetHandle() != VK_NULL_HANDLE);
        CHECK(queue->GetFamily() == ctx.GetGraphicsFamily());
    }

    SECTION("Compute queue has valid handle and family")
    {
        auto *queue = ctx.GetComputeQueue();
        REQUIRE(queue != nullptr);

        CHECK(queue->GetHandle() != VK_NULL_HANDLE);
        CHECK(queue->GetFamily() == ctx.GetComputeFamily());
    }

    SECTION("Transfer queue has valid handle and family")
    {
        auto *queue = ctx.GetTransferQueue();
        REQUIRE(queue != nullptr);

        CHECK(queue->GetHandle() != VK_NULL_HANDLE);
        CHECK(queue->GetFamily() == ctx.GetTransferFamily());
    }
}

TEST_CASE("Queue - ToString", "[queue][utility]")
{
    CHECK(std::string(VKit::ToString(VKit::Queue_Graphics)) == "Graphics");
    CHECK(std::string(VKit::ToString(VKit::Queue_Compute)) == "Compute");
    CHECK(std::string(VKit::ToString(VKit::Queue_Transfer)) == "Transfer");
    CHECK(std::string(VKit::ToString(VKit::Queue_Present)) == "Present");
}

// ============================================================================
// QUEUE - SUBMIT TESTS
// ============================================================================

TEST_CASE("Queue::Submit - Basic Submission", "[queue][submit]")
{
    ContextGuard guard;
    auto &ctx = TestContext::Get();
    auto proxy = ctx.GetProxy();
    auto *queue = ctx.GetGraphicsQueue();
    REQUIRE(queue != nullptr);

    auto poolResult =
        VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    REQUIRE(poolResult);
    auto pool = poolResult.GetValue();

    SECTION("Submit single command buffer")
    {
        auto allocResult = pool.Allocate();
        REQUIRE(allocResult);
        VkCommandBuffer cmd = allocResult.GetValue();

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        REQUIRE(proxy.Table->BeginCommandBuffer(cmd, &beginInfo) == VK_SUCCESS);
        REQUIRE(proxy.Table->EndCommandBuffer(cmd) == VK_SUCCESS);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        queue->NextTimelineValue();
        auto submitResult = queue->Submit(submitInfo);
        REQUIRE(submitResult);

        auto waitResult = queue->WaitIdle();
        REQUIRE(waitResult);

        pool.Deallocate(cmd);
    }

    SECTION("Submit multiple command buffers in one submit")
    {
        constexpr u32 count = 4;
        TKit::FixedArray<VkCommandBuffer, count> cmds;

        auto allocResult = pool.Allocate(TKit::Span<VkCommandBuffer>(cmds.GetData(), count));
        REQUIRE(allocResult);

        for (u32 i = 0; i < count; ++i)
        {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            REQUIRE(proxy.Table->BeginCommandBuffer(cmds[i], &beginInfo) == VK_SUCCESS);
            REQUIRE(proxy.Table->EndCommandBuffer(cmds[i]) == VK_SUCCESS);
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = count;
        submitInfo.pCommandBuffers = cmds.GetData();

        queue->NextTimelineValue();
        auto submitResult = queue->Submit(submitInfo);
        REQUIRE(submitResult);

        auto waitResult = queue->WaitIdle();
        REQUIRE(waitResult);

        pool.Deallocate(TKit::Span<const VkCommandBuffer>(cmds.GetData(), count));
    }

    SECTION("Submit multiple batches (span overload)")
    {
        constexpr u32 batchCount = 3;
        TKit::FixedArray<VkCommandBuffer, batchCount> cmds;
        TKit::FixedArray<VkSubmitInfo, batchCount> submitInfos;

        auto allocResult = pool.Allocate(TKit::Span<VkCommandBuffer>(cmds.GetData(), batchCount));
        REQUIRE(allocResult);

        for (u32 i = 0; i < batchCount; ++i)
        {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            REQUIRE(proxy.Table->BeginCommandBuffer(cmds[i], &beginInfo) == VK_SUCCESS);
            REQUIRE(proxy.Table->EndCommandBuffer(cmds[i]) == VK_SUCCESS);

            VkSubmitInfo &submit = submitInfos[i];
            submit = {};
            submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit.commandBufferCount = 1;
            submit.pCommandBuffers = &cmds[i];
        }

        queue->NextTimelineValue();
        auto submitResult = queue->Submit(TKit::Span<const VkSubmitInfo>(submitInfos.GetData(), batchCount));
        REQUIRE(submitResult);

        auto waitResult = queue->WaitIdle();
        REQUIRE(waitResult);

        pool.Deallocate(TKit::Span<const VkCommandBuffer>(cmds.GetData(), batchCount));
    }

    pool.Destroy();
}

TEST_CASE("Queue::Submit - With Fence", "[queue][submit][fence]")
{
    ContextGuard guard;
    auto &ctx = TestContext::Get();
    auto proxy = ctx.GetProxy();
    auto *queue = ctx.GetGraphicsQueue();
    REQUIRE(queue != nullptr);

    auto poolResult =
        VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    REQUIRE(poolResult);
    auto pool = poolResult.GetValue();

    SECTION("Submit with unsignaled fence and wait")
    {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = 0; // Unsignaled

        VkFence fence;
        REQUIRE(proxy.Table->CreateFence(proxy, &fenceInfo, proxy.AllocationCallbacks, &fence) == VK_SUCCESS);

        auto allocResult = pool.Allocate();
        REQUIRE(allocResult);
        VkCommandBuffer cmd = allocResult.GetValue();

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        REQUIRE(proxy.Table->BeginCommandBuffer(cmd, &beginInfo) == VK_SUCCESS);
        REQUIRE(proxy.Table->EndCommandBuffer(cmd) == VK_SUCCESS);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        queue->NextTimelineValue();
        auto submitResult = queue->Submit(submitInfo, fence);
        REQUIRE(submitResult);

        // Wait for fence
        VkResult waitResult = proxy.Table->WaitForFences(proxy, 1, &fence, VK_TRUE, UINT64_MAX);
        CHECK(waitResult == VK_SUCCESS);

        // Check fence is signaled
        CHECK(proxy.Table->GetFenceStatus(proxy, fence) == VK_SUCCESS);

        pool.Deallocate(cmd);
        proxy.Table->DestroyFence(proxy, fence, proxy.AllocationCallbacks);
    }

    SECTION("Multiple submissions reusing fence with reset")
    {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        VkFence fence;
        REQUIRE(proxy.Table->CreateFence(proxy, &fenceInfo, proxy.AllocationCallbacks, &fence) == VK_SUCCESS);

        auto allocResult = pool.Allocate();
        REQUIRE(allocResult);
        VkCommandBuffer cmd = allocResult.GetValue();

        for (int i = 0; i < 5; ++i)
        {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            REQUIRE(proxy.Table->BeginCommandBuffer(cmd, &beginInfo) == VK_SUCCESS);
            REQUIRE(proxy.Table->EndCommandBuffer(cmd) == VK_SUCCESS);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cmd;

            queue->NextTimelineValue();
            auto submitResult = queue->Submit(submitInfo, fence);
            REQUIRE(submitResult);

            REQUIRE(proxy.Table->WaitForFences(proxy, 1, &fence, VK_TRUE, UINT64_MAX) == VK_SUCCESS);
            REQUIRE(proxy.Table->ResetFences(proxy, 1, &fence) == VK_SUCCESS);
            REQUIRE(proxy.Table->ResetCommandBuffer(cmd, 0) == VK_SUCCESS);
        }

        pool.Deallocate(cmd);
        proxy.Table->DestroyFence(proxy, fence, proxy.AllocationCallbacks);
    }

    pool.Destroy();
}

// ============================================================================
// QUEUE - WAIT IDLE TESTS
// ============================================================================

TEST_CASE("Queue::WaitIdle", "[queue][wait]")
{
    ContextGuard guard;
    auto &ctx = TestContext::Get();
    auto proxy = ctx.GetProxy();
    auto *queue = ctx.GetGraphicsQueue();
    REQUIRE(queue != nullptr);

    SECTION("WaitIdle on empty queue succeeds")
    {
        auto result = queue->WaitIdle();
        REQUIRE(result);
    }

    SECTION("WaitIdle after submission")
    {
        auto poolResult = VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), 0);
        REQUIRE(poolResult);
        auto pool = poolResult.GetValue();

        auto allocResult = pool.Allocate();
        REQUIRE(allocResult);
        VkCommandBuffer cmd = allocResult.GetValue();

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        REQUIRE(proxy.Table->BeginCommandBuffer(cmd, &beginInfo) == VK_SUCCESS);
        REQUIRE(proxy.Table->EndCommandBuffer(cmd) == VK_SUCCESS);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        queue->NextTimelineValue();
        auto submitResult = queue->Submit(submitInfo);
        REQUIRE(submitResult);

        auto waitResult = queue->WaitIdle();
        REQUIRE(waitResult);

        pool.Deallocate(cmd);
        pool.Destroy();
    }

    SECTION("Multiple consecutive WaitIdle calls")
    {
        for (int i = 0; i < 10; ++i)
        {
            auto result = queue->WaitIdle();
            REQUIRE(result);
        }
    }
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

TEST_CASE("Integration - Pool and Queue Stress Test", "[integration][stress]")
{
    ContextGuard guard;
    auto &ctx = TestContext::Get();
    auto proxy = ctx.GetProxy();
    auto *queue = ctx.GetGraphicsQueue();
    REQUIRE(queue != nullptr);

    auto poolResult =
        VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    REQUIRE(poolResult);
    auto pool = poolResult.GetValue();

    SECTION("Rapid allocate-submit-wait cycles")
    {
        constexpr int cycles = 50;

        for (int i = 0; i < cycles; ++i)
        {
            auto beginResult = pool.BeginSingleTimeCommands();
            REQUIRE(beginResult);

            auto endResult = pool.EndSingleTimeCommands(beginResult.GetValue(), *queue);
            REQUIRE(endResult);
        }
    }

    SECTION("Batch operations with periodic resets")
    {
        constexpr int batches = 10;
        constexpr u32 buffersPerBatch = 8;

        for (int batch = 0; batch < batches; ++batch)
        {
            TKit::FixedArray<VkCommandBuffer, buffersPerBatch> cmds;

            auto allocResult = pool.Allocate(TKit::Span<VkCommandBuffer>(cmds.GetData(), buffersPerBatch));
            REQUIRE(allocResult);

            for (u32 i = 0; i < buffersPerBatch; ++i)
            {
                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                REQUIRE(proxy.Table->BeginCommandBuffer(cmds[i], &beginInfo) == VK_SUCCESS);
                REQUIRE(proxy.Table->EndCommandBuffer(cmds[i]) == VK_SUCCESS);
            }

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = buffersPerBatch;
            submitInfo.pCommandBuffers = cmds.GetData();

            queue->NextTimelineValue();
            auto submitResult = queue->Submit(submitInfo);
            REQUIRE(submitResult);

            auto waitResult = queue->WaitIdle();
            REQUIRE(waitResult);

            auto resetResult = pool.Reset(VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
            REQUIRE(resetResult);
        }
    }

    pool.Destroy();
}

TEST_CASE("Integration - Multi-Queue Operations", "[integration][multi_queue]")
{
    ContextGuard guard;
    auto &ctx = TestContext::Get();
    auto proxy = ctx.GetProxy();

    auto *graphicsQueue = ctx.GetGraphicsQueue();
    auto *computeQueue = ctx.GetComputeQueue();
    auto *transferQueue = ctx.GetTransferQueue();

    REQUIRE(graphicsQueue != nullptr);
    REQUIRE(computeQueue != nullptr);
    REQUIRE(transferQueue != nullptr);

    auto graphicsPoolResult =
        VKit::CommandPool::Create(proxy, ctx.GetGraphicsFamily(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    auto computePoolResult =
        VKit::CommandPool::Create(proxy, ctx.GetComputeFamily(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    auto transferPoolResult =
        VKit::CommandPool::Create(proxy, ctx.GetTransferFamily(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    REQUIRE(graphicsPoolResult);
    REQUIRE(computePoolResult);
    REQUIRE(transferPoolResult);

    auto graphicsPool = graphicsPoolResult.GetValue();
    auto computePool = computePoolResult.GetValue();
    auto transferPool = transferPoolResult.GetValue();

    SECTION("Submit to multiple queues independently")
    {
        // Submit to graphics
        auto gBegin = graphicsPool.BeginSingleTimeCommands();
        REQUIRE(gBegin);
        auto gEnd = graphicsPool.EndSingleTimeCommands(gBegin.GetValue(), *graphicsQueue);
        REQUIRE(gEnd);

        // Submit to compute
        auto cBegin = computePool.BeginSingleTimeCommands();
        REQUIRE(cBegin);
        auto cEnd = computePool.EndSingleTimeCommands(cBegin.GetValue(), *computeQueue);
        REQUIRE(cEnd);

        // Submit to transfer
        auto tBegin = transferPool.BeginSingleTimeCommands();
        REQUIRE(tBegin);
        auto tEnd = transferPool.EndSingleTimeCommands(tBegin.GetValue(), *transferQueue);
        REQUIRE(tEnd);
    }

    graphicsPool.Destroy();
    computePool.Destroy();
    transferPool.Destroy();
}
