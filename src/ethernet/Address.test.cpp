#include <doctest/doctest.h>
#include <ethernet/Address.hpp>

namespace
{
// NOLINTBEGIN(*)
using namespace monkas::ethernet;

TEST_SUITE("[ethernet::Address]")
{
    std::array<uint8_t, ADDR_LEN> bytesNull {0, 0, 0, 0, 0, 0};
    std::array<uint8_t, ADDR_LEN> bytesSome {1, 2, 3, 4, 5, 0x1A};

    const auto someAddress = Address::fromBytes(bytesSome);
    const auto nullAddress = Address::fromBytes(bytesNull);
    const auto defaultAddress = Address {};

    TEST_CASE("toString")
    {
        CHECK(defaultAddress.toString() == "Invalid");
        CHECK(nullAddress.toString() == "00:00:00:00:00:00");
        CHECK(someAddress.toString() == "01:02:03:04:05:1a");
    }

    TEST_CASE("operator bool")
    {
        CHECK(!defaultAddress);
        CHECK(nullAddress);
        CHECK(someAddress);
    }

    TEST_CASE("operator ==")
    {
        CHECK_FALSE(defaultAddress == nullAddress);
        CHECK(someAddress == someAddress);
    }

    TEST_CASE("operator !=")
    {
        CHECK(defaultAddress != nullAddress);
        CHECK(defaultAddress != someAddress);
        CHECK(nullAddress != someAddress);
    }

    TEST_CASE("operator <")
    {
        CHECK(defaultAddress < nullAddress);
        CHECK(defaultAddress < someAddress);
        CHECK(nullAddress < someAddress);
    }
}

// NOLINTEND(*)
}  // namespace
