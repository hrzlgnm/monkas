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

    const auto someAddress = Address {bytesSome};
    const auto nullAddress = Address {bytesNull};

    TEST_CASE("toString")
    {
        CHECK(nullAddress.toString() == "00:00:00:00:00:00");
        CHECK(someAddress.toString() == "01:02:03:04:05:1a");
    }

    TEST_CASE("operator ==")
    {
        CHECK(someAddress == someAddress);
        CHECK(nullAddress == nullAddress);
    }

    TEST_CASE("operator !=")
    {
        CHECK(nullAddress != someAddress);
    }

    TEST_CASE("operator <")
    {
        CHECK(nullAddress < someAddress);
    }
}

// NOLINTEND(*)
}  // namespace
