#include "tests/utils.hpp"
#include "vkit/core/core.hpp"
#include "vkit/vulkan/vulkan.hpp"
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

namespace VKit
{
void Setup() noexcept
{
    const auto vkres = Core::Initialize();
    VKIT_LOG_RESULT(vkres);
    REQUIRE(vkres);
}
struct GlobalSetup : Catch::EventListenerBase
{
    using Catch::EventListenerBase::EventListenerBase;

    void testRunStarting(Catch::TestRunInfo const &) override
    {
        Setup();
    }

    void testRunEnded(Catch::TestRunStats const &) override
    {
    }
};
CATCH_REGISTER_LISTENER(GlobalSetup);
} // namespace VKit
