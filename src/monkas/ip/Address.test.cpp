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
    std::array<uint8_t, 4> localhost4OtherSubnet{127, 0, 1, 1};
    std::array<uint8_t, 16> localhost6{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    std::array<uint8_t, 16> localhostV4mapped{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 127, 0, 0, 1};
    std::array<uint8_t, 4> v4countUp{1, 2, 3, 4};
    std::array<uint8_t, 4> v4countDown{4, 3, 2, 1};
    std::array<uint8_t, 16> v6countUp{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    std::array<uint8_t, 16> v6countDown{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
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
        CHECK(Address::fromBytes(localhost4) == Address::fromBytes(localhostV4mapped));
        CHECK(Address::fromBytes(localhostV4mapped) == Address::fromBytes(localhost4));
        CHECK(Address::fromBytes(localhostV4mapped) == Address::fromBytes(localhostV4mapped));
    }

    TEST_CASE("operator !=")
    {
        CHECK(Address{} != Address::fromBytes(any4));
        CHECK(Address{} != Address::fromBytes(any6));
        CHECK(Address::fromBytes(any6) != Address::fromBytes(any4));
        CHECK(Address::fromBytes(localhost4) != Address::fromBytes(localhost6));
        CHECK(Address{} != Address::fromBytes(localhost4));
        CHECK(Address{} != Address::fromBytes(localhost6));
        CHECK_FALSE(Address::fromBytes(localhostV4mapped) != Address::fromBytes(localhost4));
    }

    TEST_CASE("operator <")
    {
        CHECK(Address{} < Address::fromBytes(localhost4));
        CHECK(v4countUp < v4countDown);
        CHECK(v6countUp < v6countDown);
        CHECK(Address::fromBytes(localhost4) < Address::fromBytes(localhost4OtherSubnet));
        CHECK(Address::fromBytes(localhost4) < Address::fromBytes(localhost6));
    }

    TEST_CASE("operator <=")
    {
        CHECK(Address{} <= Address{});
        CHECK(Address{} <= Address::fromBytes(localhost4));
        CHECK(v6countUp <= v6countDown);
        CHECK(v4countUp <= v4countDown);
        CHECK(Address::fromBytes(localhost4) <= Address::fromBytes(localhost4));
        CHECK(Address::fromBytes(localhost4) <= Address::fromBytes(localhostV4mapped));
        CHECK(Address::fromBytes(localhostV4mapped) <= Address::fromBytes(localhost4));
        CHECK(Address::fromBytes(localhost4) <= Address::fromBytes(localhost6));
    }

    TEST_CASE("operator >")
    {
        CHECK(v4countDown > v4countUp);
        CHECK(v6countDown > v6countUp);
        CHECK(Address::fromBytes(localhost6) > Address{});
        CHECK(Address::fromBytes(localhost4) > Address{});
        CHECK(Address::fromBytes(localhost4OtherSubnet) > Address::fromBytes(localhost4));
        CHECK(Address::fromBytes(localhost6) > Address::fromBytes(localhost4));
    }

    TEST_CASE("operator >=")
    {
        CHECK(v4countDown >= v4countUp);
        CHECK(v6countDown >= v6countUp);
        CHECK(Address::fromBytes(localhost6) >= Address{});
        CHECK(Address::fromBytes(localhost4) >= Address{});
        CHECK(Address::fromBytes(localhost4OtherSubnet) >= Address::fromBytes(localhost4));
        CHECK(Address::fromBytes(localhost6) >= Address::fromBytes(localhost4));
        CHECK(Address::fromBytes(localhostV4mapped) >= Address::fromBytes(localhost4));
        CHECK(Address::fromBytes(localhost4) >= Address::fromBytes(localhostV4mapped));
    }
}

// NOLINTEND(*)
} // namespace
