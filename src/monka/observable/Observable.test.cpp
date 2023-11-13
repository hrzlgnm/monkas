#include <doctest/doctest.h>
#include <memory>
#include <observable/Observable.h>
#include <stdexcept>

namespace
{
TEST_SUITE_BEGIN("[observable]");
TEST_CASE("Observable tests")
{
    Observable<int> o;
    int last_a = 0;
    SUBCASE("registering listener and calling broadcast calls listener")
    {
        o.addListener([&last_a](int a) { last_a = a; });
        o.broadcast(5);
        REQUIRE(last_a == 5);
    }

    SUBCASE("registering two listener and calling broadcast calls listener")
    {
        int last_b = 0;
        o.addListener([&last_a](int a) { last_a = a; });
        o.addListener([&last_b](int b) { last_b = b; });
        o.broadcast(5);
        REQUIRE(last_a == 5);
        REQUIRE(last_b == 5);
    }

    SUBCASE("unregistered listener is no called when broadcasting")
    {
        const auto t = o.addListener([&last_a](int a) { last_a = a; });
        o.removeListener(t);
        o.broadcast(5);
        REQUIRE(last_a == 0);
    }

    SUBCASE("unregistered listener during dispatch is not called when broadcasting")
    {
        int a_count = 0;
        int b_count = 0;
        auto u = std::make_shared<Observable<int>::Token>();
        o.addListener([&o, &a_count, u](int) {
            if (a_count == 0)
            {
                o.removeListener(*u);
            }
            a_count++;
        });
        *u = o.addListener([&o, &b_count](int) { b_count++; });
        o.broadcast(5);
        o.broadcast(6);
        o.broadcast(7);
        REQUIRE(a_count == 3);
        REQUIRE(b_count == 0);
    }

    SUBCASE("if one of two listeners throws the second one is still called")
    {
        o.addListener([](int) { throw std::runtime_error("banana"); });
        o.addListener([&last_a](int a) { last_a = a; });
        REQUIRE_NOTHROW(o.broadcast(5));
        REQUIRE_NOTHROW(o.broadcast(6));
        REQUIRE(last_a == 6);
    }
}
TEST_SUITE_END();
} // namespace
