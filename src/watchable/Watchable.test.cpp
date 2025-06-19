#include <doctest/doctest.h>
#include <memory>
#include <stdexcept>
#include <utility>
#include <watchable/Watchable.hpp>

namespace
{
// NOLINTBEGIN(*)
using namespace monkas;
TEST_SUITE_BEGIN("[watchable]");

TEST_CASE("Watchable tests")
{
    spdlog::set_level(spdlog::level::trace);
    Watchable<int> watchable;
    int lastA = 0;
    int lastB = 0;
    auto assignToLastA = [&lastA](int x) { lastA = x; };
    auto assignToLastB = [&lastB](int x) { lastB = x; };
    SUBCASE("registering watcher and calling notify calls watcher")
    {
        std::ignore = watchable.addWatcher(assignToLastA);

        watchable.notify(5);

        CHECK(lastA == 5);
    }

    SUBCASE("registering two watcher and calling notify calls watcher")
    {
        std::ignore = watchable.addWatcher(assignToLastA);
        std::ignore = watchable.addWatcher(assignToLastB);

        watchable.notify(5);

        CHECK(lastA == 5);
        CHECK(lastB == 5);
    }

    SUBCASE("unregistered watcher is not called when notifying")
    {
        const auto t = watchable.addWatcher(assignToLastA);

        watchable.removeWatcher(t);
        watchable.notify(5);

        CHECK(lastA == 0);
    }

    SUBCASE("unregistered watcher during notify is not called when notifying")
    {
        auto u = std::make_shared<Watchable<int>::Token>();
        std::ignore = watchable.addWatcher([&watchable, &lastA, u](int x) {
            if (lastA == 0)
            {
                watchable.removeWatcher(*u);
                // remove twice to check that it is safe
                watchable.removeWatcher(*u);
            }
            lastA = x;
        });
        *u = watchable.addWatcher(assignToLastB);

        watchable.notify(5);
        watchable.notify(6);
        watchable.notify(7);

        CHECK(lastA == 7);
        CHECK(lastB == 0);
    }

    SUBCASE("if two of three watchers throws the last one is still called")
    {
        std::ignore = watchable.addWatcher([](int) { throw std::runtime_error("banana"); });
        std::ignore = watchable.addWatcher([](int) { throw 1; });
        std::ignore = watchable.addWatcher(assignToLastA);

        CHECK_NOTHROW(watchable.notify(5));
        CHECK_NOTHROW(watchable.notify(6));

        CHECK(lastA == 6);
    }

    spdlog::set_level(spdlog::level::info);
}

TEST_SUITE_END();
// NOLINTEND(*)
} // namespace
