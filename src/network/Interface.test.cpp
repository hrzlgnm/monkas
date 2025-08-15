#include <system_error>
#include <tuple>

#include <doctest/doctest.h>
#include <network/Interface.hpp>

namespace
{
// NOLINTBEGIN(*)

using namespace monkas::network;

TEST_SUITE("[network::Interface]")
{
    Interface defaultInterface(0, "default");
    Interface someInterface(1, "some");
    Interface renamedSomeInterface(1, "renamed");

    TEST_CASE("from name")
    {
        const auto iface = Interface::fromName("lo");
        CHECK(iface.index() != 0);
        CHECK(iface.name() == "lo");
        CHECK_THROWS_AS(std::ignore = Interface::fromName("nonexistent"), std::system_error);
    }

    TEST_CASE("from index")
    {
        const auto lo = Interface::fromName("lo");
        const auto iface = Interface::fromIndex(lo.index());
        CHECK(iface.index() == lo.index());
        CHECK(iface.name() == "lo");
        CHECK_THROWS_AS(std::ignore = Interface::fromIndex(0), std::system_error);
    }

    TEST_CASE("operator ==")
    {
        CHECK(defaultInterface == defaultInterface);
        CHECK(someInterface == someInterface);
        CHECK(someInterface == renamedSomeInterface);
    }

    TEST_CASE("operator !=")
    {
        CHECK(defaultInterface != someInterface);
        CHECK(someInterface != defaultInterface);
        CHECK(defaultInterface != renamedSomeInterface);
    }

    TEST_CASE("operator <")
    {
        CHECK(defaultInterface < someInterface);
        CHECK(defaultInterface < renamedSomeInterface);
    }

    TEST_CASE("operator >=")
    {
        CHECK(someInterface >= defaultInterface);
        CHECK(renamedSomeInterface >= defaultInterface);
    }
}

// NOLINTEND(*)
}  // namespace
