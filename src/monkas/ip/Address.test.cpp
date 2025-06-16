#include <doctest/doctest.h>
#include <ip/Address.hpp>

namespace
{
// NOLINTBEGIN(*)

using namespace monkas::ip;

TEST_SUITE("[ip::Address]")
{
    std::array<uint8_t, 4> bytesAny4{};
    std::array<uint8_t, 4> bytesV4CountUp{1, 2, 3, 4};
    std::array<uint8_t, 4> bytesV4CountDown{4, 3, 2, 1};
    std::array<uint8_t, 4> bytesLocalhost4{127, 0, 0, 1};
    std::array<uint8_t, 4> bytesLocalhost4OtherSubnet{127, 0, 1, 1};

    std::array<uint8_t, 16> bytesAny6{};
    std::array<uint8_t, 16> bytesLocalHost6{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    std::array<uint8_t, 16> bytesV6CountUp{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    std::array<uint8_t, 16> bytesV6CountDown{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

    std::array<uint8_t, 16> bytesLocalhostV4mapped{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 127, 0, 0, 1};

    const auto any4 = Address::fromBytes(bytesAny4);
    const auto any6 = Address::fromBytes(bytesAny6);
    const auto localhost4 = Address::fromBytes(bytesLocalhost4);
    const auto localhost4OtherSubnet = Address::fromBytes(bytesLocalhost4OtherSubnet);
    const auto localHost6 = Address::fromBytes(bytesLocalHost6);
    const auto localhostV4mapped = Address::fromBytes(bytesLocalhostV4mapped);
    const auto countUpV4 = Address::fromBytes(bytesV4CountUp);
    const auto countDownV4 = Address::fromBytes(bytesV4CountDown);
    const auto countUpV6 = Address::fromBytes(bytesV6CountUp);
    const auto countDownV6 = Address::fromBytes(bytesV6CountDown);
    const auto unspec = Address{};

    TEST_CASE("toString")
    {
        CHECK(localhost4.toString() == "127.0.0.1");
        CHECK(localHost6.toString() == "::1");
    }

    TEST_CASE("fromString")
    {
        CHECK(Address::fromString("127.0.0.1").toString() == "127.0.0.1");
        CHECK(Address::fromString("::1").toString() == "::1");
    }

    TEST_CASE("operator ==")
    {
        CHECK(localhost4 == localhost4);
        CHECK(localhost4 == localhostV4mapped);
        CHECK(localhostV4mapped == localhost4);
        CHECK(localhostV4mapped == localhostV4mapped);
    }

    TEST_CASE("operator !=")
    {
        CHECK(unspec != any4);
        CHECK(unspec != any6);
        CHECK(any6 != any4);
        CHECK(localhost4 != localHost6);
        CHECK(unspec != localhost4);
        CHECK(unspec != localHost6);
        CHECK_FALSE(localhostV4mapped != localhost4);
    }

    TEST_CASE("operator <")
    {
        CHECK(unspec < localhost4);
        CHECK(countUpV4 < countDownV4);
        CHECK(countUpV6 < countDownV6);
        CHECK(localhost4 < localhost4OtherSubnet);
        CHECK(localhost4 < localHost6);
    }

    TEST_CASE("operator <=")
    {
        CHECK(unspec <= unspec);
        CHECK(unspec <= localhost4);
        CHECK(countUpV6 <= countDownV6);
        CHECK(countUpV4 <= countDownV4);
        CHECK(localhost4 <= localhost4);
        CHECK(localhost4 <= localhostV4mapped);
        CHECK(localhostV4mapped <= localhost4);
        CHECK(localhost4 <= localHost6);
    }

    TEST_CASE("operator >")
    {
        CHECK(countDownV4 > countUpV4);
        CHECK(countDownV6 > countUpV6);
        CHECK(localHost6 > unspec);
        CHECK(localhost4 > unspec);
        CHECK(localhost4OtherSubnet > localhost4);
        CHECK(localHost6 > localhost4);
    }

    TEST_CASE("operator >=")
    {
        CHECK(countDownV4 >= countUpV4);
        CHECK(countDownV6 >= countUpV6);
        CHECK(localHost6 >= unspec);
        CHECK(localhost4 >= unspec);
        CHECK(localhost4OtherSubnet >= localhost4);
        CHECK(localHost6 >= localhost4);
        CHECK(localhostV4mapped >= localhost4);
        CHECK(localhost4 >= localhostV4mapped);
    }

    TEST_CASE("operator bool")
    {
        CHECK(!unspec);
        CHECK(localhost4);
        CHECK(localHost6);
        CHECK(localhostV4mapped);
        CHECK(any4);
        CHECK(any6);
        CHECK(countUpV4);
        CHECK(countDownV4);
        CHECK(countUpV6);
        CHECK(countDownV6);
    }
}

// NOLINTEND(*)
} // namespace
