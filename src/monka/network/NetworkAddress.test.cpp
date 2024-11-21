#include <doctest/doctest.h>
#include <ip/Address.h>
#include <network/NetworkAddress.h>

namespace
{

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
        REQUIRE(addr);
        REQUIRE(!NetworkAddress{});
    }

    TEST_CASE("operator <")
    {
        REQUIRE(NetworkAddress{} < addr);
        REQUIRE(addr >= NetworkAddress{});
    }
}
} // namespace
