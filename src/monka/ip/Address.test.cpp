#include <doctest/doctest.h>
#include <ip/Address.h>

namespace
{

using namespace monkas::ip;
TEST_SUITE("[ip::Address]")
{
    std::array<uint8_t, 4> localhost4{127, 0, 0, 1};
    std::array<uint8_t, 16> localhost6{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};

    TEST_CASE("toString")
    {
        REQUIRE(Address::fromBytes(localhost4).toString() == "127.0.0.1");
        REQUIRE(Address::fromBytes(localhost6).toString() == "::1");
    }

    TEST_CASE("operator ==")
    {
        REQUIRE(Address::fromBytes(localhost4) == Address::fromBytes(localhost4));
    }

    TEST_CASE("operator !=")
    {
        REQUIRE(Address{} != Address::fromBytes(localhost4));

        REQUIRE(Address{} != Address::fromBytes(localhost6));
        REQUIRE(Address::fromBytes(localhost4) != Address::fromBytes(localhost6));
    }

    TEST_CASE("operator <")
    {
        REQUIRE(Address{} < Address::fromBytes(localhost4));

        std::array<uint8_t, 4> localhost4_other{127, 0, 1, 1};
        REQUIRE(Address::fromBytes(localhost4) < Address::fromBytes(localhost4_other));

        REQUIRE(Address::fromBytes(localhost4) < Address::fromBytes(localhost6));
    }
}
} // namespace
