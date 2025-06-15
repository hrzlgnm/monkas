#include <doctest/doctest.h>
#include <ip/Address.hpp>

namespace
{
// NOLINTBEGIN(*)

using namespace monkas::ip;

TEST_SUITE("[ip::Address]")
{
    std::array<uint8_t, 4> any4;
    std::array<uint8_t, 16> any6;

    std::array<uint8_t, 4> localhost4{127, 0, 0, 1};
    std::array<uint8_t, 16> localhost6{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};

    TEST_CASE("toString")
    {
        CHECK(Address::fromBytes(localhost4).toString() == "127.0.0.1");
        CHECK(Address::fromBytes(localhost6).toString() == "::1");
    }

    TEST_CASE("fromString")
    {
        CHECK(Address::fromString("127.0.0.1").toString() == "127.0.0.1");
        CHECK(Address::fromString("::1").toString() == "::1");
    }

    TEST_CASE("operator ==")
    {
        CHECK(Address::fromBytes(localhost4) == Address::fromBytes(localhost4));
    }

    TEST_CASE("operator !=")
    {
        CHECK(Address{} != Address::fromBytes(any4));
        CHECK(Address{} != Address::fromBytes(any6));
        CHECK(Address::fromBytes(any6) != Address::fromBytes(any4));
        CHECK(Address::fromBytes(localhost4) != Address::fromBytes(localhost6));
        CHECK(Address{} != Address::fromBytes(localhost4));
        CHECK(Address{} != Address::fromBytes(localhost6));
    }

    TEST_CASE("operator <")
    {
        CHECK(Address{} < Address::fromBytes(localhost4));

        std::array<uint8_t, 4> localhost4_other{127, 0, 1, 1};
        CHECK(Address::fromBytes(localhost4) < Address::fromBytes(localhost4_other));

        CHECK(Address::fromBytes(localhost4) < Address::fromBytes(localhost6));
    }
}

// NOLINTEND(*)
} // namespace
