#include <doctest/doctest.h>
#include <ethernet/Address.h>

namespace
{
using namespace monkas::ethernet;
TEST_SUITE("[ethernet::Address]")
{

    std::array<uint8_t, 6> null{0, 0, 0, 0, 0, 0};
    std::array<uint8_t, 6> some{1, 2, 3, 4, 5, 0x1A};
    TEST_CASE("toString")
    {
        REQUIRE(Address::fromBytes(null).toString() == "00:00:00:00:00:00");
        REQUIRE(Address::fromBytes(some).toString() == "01:02:03:04:05:1a");
    }

    TEST_CASE("operator ==")
    {
        REQUIRE(Address{} == Address::fromBytes(null));
        REQUIRE(Address::fromBytes(null) == Address::fromBytes(null));
        REQUIRE(Address::fromBytes(some) == Address::fromBytes(some));
    }

    TEST_CASE("operator !=")
    {
        REQUIRE(Address{} != Address::fromBytes(some));
        REQUIRE(Address::fromBytes(null) != Address::fromBytes(some));
    }

    TEST_CASE("operator <")
    {
        REQUIRE(Address{} < Address::fromBytes(some));

        REQUIRE(Address::fromBytes(null) < Address::fromBytes(some));
    }
    TEST_CASE("operator bool")
    {
        REQUIRE(!Address{});
        REQUIRE(!Address::fromBytes(null));
        REQUIRE(Address::fromBytes(some));
    }
}
} // namespace
