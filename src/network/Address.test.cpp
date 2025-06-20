#include <doctest/doctest.h>
#include <ip/Address.hpp>
#include <network/Address.hpp>

namespace
{
// NOLINTBEGIN(*)

using namespace monkas;
using namespace monkas::network;

TEST_SUITE("[network::Address]")
{

    Scope scope{Scope::Global};
    std::array<uint8_t, 6> some_hw_addr{1, 2, 3, 4, 5, 0x1A};
    std::array<uint8_t, 4> some_ipv4{192, 168, 17, 1};
    std::array<uint8_t, 4> some_bcast{192, 168, 17, 255};

    const auto someV4 = ip::Address::fromBytes(some_ipv4);
    const auto someBcastV4 = ip::Address::fromBytes(some_bcast);
    const auto someV6 = ip::Address::fromString("2001:db8::1");
    const Address addrV6{someV6, someBcastV4, 24, scope, 5, 0};
    const Address addrV4{someV4, someBcastV4, 24, scope, 10, 1};

    TEST_CASE("proto")
    {
        CHECK(addrV4.proto() == 1);
        CHECK(addrV6.proto() == 0);
        CHECK(defaultAddress.proto() == 0);
    }
    const Address defaultAddress{};

    TEST_CASE("operator bool")
    {
        CHECK(addrV4);
        CHECK(!defaultAddress);
    }

    TEST_CASE("family")
    {
        CHECK(addrV4.family() == someV4.family());
        CHECK(defaultAddress.family() == network::Family::Unspecified);
    }

    TEST_CASE("isV4")
    {
        CHECK(addrV4.isV4());
        CHECK(!defaultAddress.isV4());
    }

    TEST_CASE("isV6")
    {
        CHECK(!addrV4.isV6());
        CHECK(addrV6.isV6());
        CHECK(!defaultAddress.isV6());
    }

    TEST_CASE("isUnspecified")
    {
        CHECK(defaultAddress.isUnspecified());
        CHECK(!addrV6.isUnspecified());
        CHECK(!addrV4.isUnspecified());
    }

    TEST_CASE("isMappedV4")
    {
        CHECK(!addrV4.isMappedV4());
        CHECK(!defaultAddress.isMappedV4());
    }

    TEST_CASE("ip")
    {
        CHECK(addrV4.ip() == someV4);
        CHECK(defaultAddress.ip() == ip::Address{});
    }

    TEST_CASE("broadcast")
    {
        CHECK(addrV4.broadcast() == someBcastV4);
        CHECK(defaultAddress.broadcast() == ip::Address{});
    }

    TEST_CASE("prefixLength")
    {
        CHECK(addrV4.prefixLength() == 24);
        CHECK(defaultAddress.prefixLength() == 0);
    }

    TEST_CASE("scope")
    {
        CHECK(addrV4.scope() == scope);
        CHECK(defaultAddress.scope() == network::Scope::Nowhere);
    }

    TEST_CASE("flags")
    {
        CHECK(addrV4.flags() == 10);
        CHECK(defaultAddress.flags() == 0);
    }

    TEST_CASE("operator ==")
    {
        CHECK(defaultAddress == defaultAddress);
        CHECK(addrV4 == addrV4);
    }

    TEST_CASE("operator !=")
    {
        CHECK(defaultAddress != addrV4);
    }

    TEST_CASE("operator <")
    {
        CHECK(defaultAddress < addrV4);
        CHECK(addrV4 >= defaultAddress);
    }

    TEST_CASE("operator >=")
    {
        CHECK(addrV4 >= defaultAddress);
    }
}

// NOLINTEND(*)
} // namespace
