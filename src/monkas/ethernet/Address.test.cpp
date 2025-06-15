#include <doctest/doctest.h>
#include <ethernet/Address.hpp>

namespace
{
// NOLINTBEGIN(*)
using namespace monkas::ethernet;

TEST_SUITE("[ethernet::Address]")
{

    std::array<uint8_t, addrLen> null{0, 0, 0, 0, 0, 0};
    std::array<uint8_t, addrLen> some{1, 2, 3, 4, 5, 0x1A};
    TEST_CASE("toString")
    {
        CHECK(Address::fromBytes(null).toString() == "00:00:00:00:00:00");
        CHECK(Address::fromBytes(some).toString() == "01:02:03:04:05:1a");
    }

    TEST_CASE("operator ==")
    {
        CHECK(Address{} == Address::fromBytes(null));
        CHECK(Address::fromBytes(null) == Address::fromBytes(null));
        CHECK(Address::fromBytes(some) == Address::fromBytes(some));
    }

    TEST_CASE("operator !=")
    {
        CHECK(Address{} != Address::fromBytes(some));
        CHECK(Address::fromBytes(null) != Address::fromBytes(some));
    }

    TEST_CASE("operator <")
    {
        CHECK(Address{} < Address::fromBytes(some));

        CHECK(Address::fromBytes(null) < Address::fromBytes(some));
    }

    TEST_CASE("operator bool")
    {
        CHECK(!Address{});
        CHECK(!Address::fromBytes(null));
        CHECK(Address::fromBytes(some));
    }
}

// NOLINTEND(*)
} // namespace
