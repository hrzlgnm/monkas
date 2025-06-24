#include <doctest/doctest.h>
#include <ip/Address.hpp>

namespace
{
// NOLINTBEGIN(*)

using namespace monkas::ip;

TEST_SUITE("[ip::Address]")
{
    std::array<uint8_t, 4> bytesAny4 {};
    std::array<uint8_t, 4> bytesV4CountUp {1, 2, 3, 4};
    std::array<uint8_t, 4> bytesV4CountDown {4, 3, 2, 1};
    std::array<uint8_t, 4> bytesLocalhost4 {127, 0, 0, 1};
    std::array<uint8_t, 4> bytesLocalhost4OtherSubnet {127, 0, 1, 1};

    std::array<uint8_t, 16> bytesAny6 {};
    std::array<uint8_t, 16> bytesLocalHost6 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    std::array<uint8_t, 16> bytesV6CountUp {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    std::array<uint8_t, 16> bytesV6CountDown {16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

    const auto any4 = Address(bytesAny4);
    const auto any6 = Address(bytesAny6);
    const auto localhost4 = Address(bytesLocalhost4);
    const auto localhost4OtherSubnet = Address(bytesLocalhost4OtherSubnet);
    const auto localHost6 = Address(bytesLocalHost6);
    const auto countUpV4 = Address(bytesV4CountUp);
    const auto countDownV4 = Address(bytesV4CountDown);
    const auto countUpV6 = Address(bytesV6CountUp);
    const auto countDownV6 = Address(bytesV6CountDown);
    const auto unspecified = Address {};

    TEST_CASE("toString")
    {
        CHECK(localhost4.toString() == "127.0.0.1");
        CHECK(localHost6.toString() == "::1");
    }

    TEST_CASE("fromString")
    {
        CHECK(Address::fromString("127.0.0.1").toString() == "127.0.0.1");
        CHECK(Address::fromString("::1").toString() == "::1");
        CHECK(Address::fromString("garbage").toString() == "Unspecified");
    }

    TEST_CASE("addressFamily")
    {
        CHECK(localhost4.family() == Family::IPv4);
        CHECK(localHost6.family() == Family::IPv6);
    }

    TEST_CASE("isV4")
    {
        CHECK(localhost4.isV4());
        CHECK(!localHost6.isV4());
    }

    TEST_CASE("isV6")
    {
        CHECK(!localhost4.isV6());
        CHECK(localHost6.isV6());
    }

    TEST_CASE("isUnspecified")
    {
        CHECK(unspecified.isUnspecified());
        CHECK(!localhost4.isUnspecified());
        CHECK(!localHost6.isUnspecified());
    }

    TEST_CASE("isMulticast")
    {
        CHECK(Address::fromString("224.0.0.1").isMulticast());
        CHECK(Address::fromString("239.255.255.253").isMulticast());
        CHECK(Address::fromString("ff02::1").isMulticast());
        CHECK(Address::fromString("ff02::2").isMulticast());
        CHECK(Address::fromString("ff02::3").isMulticast());
        CHECK(Address::fromString("ff02::4").isMulticast());
        CHECK(!unspecified.isMulticast());
        CHECK(!localhost4.isMulticast());
        CHECK(!localHost6.isMulticast());
        CHECK(!any4.isMulticast());
        CHECK(!any6.isMulticast());
    }

    TEST_CASE("isLinkLocal")
    {
        CHECK(Address::fromString("169.254.0.1").isLinkLocal());
        CHECK(Address::fromString("169.254.255.255").isLinkLocal());
        CHECK(Address::fromString("fe80::1").isLinkLocal());
        CHECK(Address::fromString("fe80::2").isLinkLocal());
        CHECK(Address::fromString("fe80::3").isLinkLocal());
        CHECK(!unspecified.isLinkLocal());
        CHECK(!localhost4.isLinkLocal());
        CHECK(!localHost6.isLinkLocal());
        CHECK(!any4.isLinkLocal());
        CHECK(!any6.isLinkLocal());
    }

    TEST_CASE("isUniqueLocal")
    {
        CHECK(Address::fromString("fc00::1").isUniqueLocal());
        CHECK(Address::fromString("fd00::1").isUniqueLocal());
        CHECK(Address::fromString("fc00:1234:5678:9abc:def0:1234:5678:9abc").isUniqueLocal());
        CHECK(!unspecified.isUniqueLocal());
        CHECK(!localhost4.isUniqueLocal());
        CHECK(!localHost6.isUniqueLocal());
        CHECK(!any4.isUniqueLocal());
        CHECK(!any6.isUniqueLocal());
    }

    TEST_CASE("isBroadcast")
    {
        CHECK(Address::fromString("255.255.255.255").isBroadcast());
        CHECK(!Address::fromString("192.168.1.1").isBroadcast());
        CHECK(!localhost4.isBroadcast());
        CHECK(!localHost6.isBroadcast());
        CHECK(!any4.isBroadcast());
        CHECK(!any6.isBroadcast());
    }

    TEST_CASE("isLoopback")
    {
        CHECK(localhost4.isLoopback());
        CHECK(localHost6.isLoopback());
        CHECK(Address::fromString("127.253.253.123").isLoopback());
    }

    TEST_CASE("isPrivate")
    {
        CHECK(Address::fromString("192.168.0.1").isPrivate());
        CHECK(Address::fromString("172.16.0.1").isPrivate());
        CHECK(Address::fromString("10.0.0.1").isPrivate());
    }

    TEST_CASE("isDocumentation")
    {
        CHECK(Address::fromString("192.0.2.1").isDocumentation());
        CHECK(Address::fromString("198.51.100.1").isDocumentation());
        CHECK(Address::fromString("203.0.113.1").isDocumentation());
    }

    TEST_CASE("operator bool")
    {
        CHECK(!unspecified);
        CHECK(localhost4);
        CHECK(localHost6);
        CHECK(any4);
        CHECK(any6);
        CHECK(countUpV4);
        CHECK(countDownV4);
        CHECK(countUpV6);
        CHECK(countDownV6);
    }

    TEST_CASE("operator ==")
    {
        CHECK(unspecified == unspecified);
        CHECK(localhost4 == localhost4);
        CHECK(localHost6 == localHost6);
    }

    TEST_CASE("operator !=")
    {
        CHECK(unspecified != any4);
        CHECK(unspecified != any6);
        CHECK(any6 != any4);
        CHECK(localhost4 != localHost6);
        CHECK(unspecified != localhost4);
        CHECK(unspecified != localHost6);
    }

    TEST_CASE("operator <")
    {
        CHECK(unspecified < localhost4);
        CHECK(countUpV4 < countDownV4);
        CHECK(countUpV6 < countDownV6);
        CHECK(localhost4 < localhost4OtherSubnet);
        CHECK(localhost4 < localHost6);
    }

    TEST_CASE("operator <=")
    {
        CHECK(unspecified <= unspecified);
        CHECK(unspecified <= localhost4);
        CHECK(countUpV6 <= countDownV6);
        CHECK(countUpV4 <= countDownV4);
        CHECK(localhost4 <= localhost4);
        CHECK(localhost4 <= localHost6);
    }

    TEST_CASE("operator >")
    {
        CHECK(countDownV4 > countUpV4);
        CHECK(countDownV6 > countUpV6);
        CHECK(localHost6 > unspecified);
        CHECK(localhost4 > unspecified);
        CHECK(localhost4OtherSubnet > localhost4);
        CHECK(localHost6 > localhost4);
    }

    TEST_CASE("operator >=")
    {
        CHECK(countDownV4 >= countUpV4);
        CHECK(countDownV6 >= countUpV6);
        CHECK(localHost6 >= unspecified);
        CHECK(localhost4 >= unspecified);
        CHECK(localhost4OtherSubnet >= localhost4);
        CHECK(localHost6 >= localhost4);
    }
}

// NOLINTEND(*)
}  // namespace
