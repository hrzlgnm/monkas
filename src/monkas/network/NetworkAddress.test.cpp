#include <doctest/doctest.h>
#include <ip/Address.hpp>
#include <network/NetworkAddress.hpp>

namespace
{
// NOLINTBEGIN(*)

using namespace monkas::network;
using namespace monkas::ip;

TEST_SUITE("[network::NetworkAddress]")
{

    AddressFamily fam{AddressFamily::IPv4};
    AddressScope scope{AddressScope::Global};
    std::array<uint8_t, 6> some_hw_addr{1, 2, 3, 4, 5, 0x1A};
    std::array<uint8_t, 4> some_ipv4{192, 168, 17, 1};
    std::array<uint8_t, 4> some_bcast{192, 168, 17, 255};
    NetworkAddress addr{fam, Address::fromBytes(some_ipv4), Address::fromBytes(some_bcast), 24, scope, 10};

    TEST_CASE("constructor")
    {
        CHECK(addr);
        CHECK(!NetworkAddress{});
    }

    TEST_CASE("operator ==")
    {
        CHECK(NetworkAddress{} == NetworkAddress{});
        CHECK(addr == addr);
    }

    TEST_CASE("operator !=")
    {
        CHECK(NetworkAddress{} != addr);
    }

    TEST_CASE("operator <")
    {
        CHECK(NetworkAddress{} < addr);
        CHECK(addr >= NetworkAddress{});
    }

    TEST_CASE("operator <")
    {
        CHECK(NetworkAddress{} < addr);
        CHECK(addr >= NetworkAddress{});
    }
}

// NOLINTEND(*)
} // namespace
