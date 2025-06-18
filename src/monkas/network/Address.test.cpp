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
    const Address addr{someV4, someBcastV4, 24, scope, 10};
    const Address defaultAddress{};

    TEST_CASE("operator bool")
    {
        CHECK(addr);
        CHECK(!defaultAddress);
    }

    TEST_CASE("family")
    {
        CHECK(addr.family() == someV4.family());
        CHECK(defaultAddress.family() == network::Family::Unspecified);
    }

    TEST_CASE("ip")
    {
        CHECK(addr.ip() == someV4);
        CHECK(defaultAddress.ip() == ip::Address{});
    }

    TEST_CASE("broadcast")
    {
        CHECK(addr.broadcast() == someBcastV4);
        CHECK(defaultAddress.broadcast() == ip::Address{});
    }

    TEST_CASE("prefixLength")
    {
        CHECK(addr.prefixLength() == 24);
        CHECK(defaultAddress.prefixLength() == 0);
    }

    TEST_CASE("scope")
    {
        CHECK(addr.scope() == scope);
        CHECK(defaultAddress.scope() == network::Scope::Nowhere);
    }

    TEST_CASE("flags")
    {
        CHECK(addr.flags() == 10);
        CHECK(defaultAddress.flags() == 0);
    }

    TEST_CASE("operator ==")
    {
        CHECK(defaultAddress == defaultAddress);
        CHECK(addr == addr);
    }

    TEST_CASE("operator !=")
    {
        CHECK(defaultAddress != addr);
    }

    TEST_CASE("operator <")
    {
        CHECK(defaultAddress < addr);
        CHECK(addr >= defaultAddress);
    }

    TEST_CASE("operator >=")
    {
        CHECK(addr >= defaultAddress);
    }
}

// NOLINTEND(*)
} // namespace
