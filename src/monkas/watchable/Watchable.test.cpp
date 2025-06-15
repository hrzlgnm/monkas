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
    Watchable<int> o;
    int last_a = 0;
    SUBCASE("registering watcher and calling notify calls watcher")
    {
        std::ignore = o.addWatcher([&last_a](int a) { last_a = a; });
        o.notify(5);
        CHECK(last_a == 5);
    }

    SUBCASE("registering two watcher and calling notify calls watcher")
    {
        int last_b = 0;
        std::ignore = o.addWatcher([&last_a](int a) { last_a = a; });
        std::ignore = o.addWatcher([&last_b](int b) { last_b = b; });
        o.notify(5);
        CHECK(last_a == 5);
        CHECK(last_b == 5);
    }

    SUBCASE("unregistered watcher is not called when notifying")
    {
        const auto t = o.addWatcher([&last_a](int a) { last_a = a; });
        o.removeWatcher(t);
        o.notify(5);
        CHECK(last_a == 0);
    }

    SUBCASE("unregistered watcher during notify is not called when notifying")
    {
        int a = 0;
        int b = 0;
        auto u = std::make_shared<Watchable<int>::Token>();
        std::ignore = o.addWatcher([&o, &a, u](int c) {
            spdlog::debug("c: {}", c);
            if (a == 0)
            {
                o.removeWatcher(*u);
                o.removeWatcher(*u);
            }
            a = c;
        });
        *u = o.addWatcher([&o, &b](int c) {
            spdlog::debug("c: {}", c);
            b = c;
        });
        o.notify(5);
        o.notify(6);
        o.notify(7);
        CHECK(a == 7);
        CHECK(b == 0);
    }

    SUBCASE("if two of three watchers throws the last one is still called")
    {
        std::ignore = o.addWatcher([](int) { throw std::runtime_error("banana"); });
        std::ignore = o.addWatcher([](int) { throw 1; });
        std::ignore = o.addWatcher([&last_a](int a) { last_a = a; });
        CHECK_NOTHROW(o.notify(5));
        CHECK_NOTHROW(o.notify(6));
        CHECK(last_a == 6);
    }
}

TEST_SUITE_END();
// NOLINTEND(*)
} // namespace
