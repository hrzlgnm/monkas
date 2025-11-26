// Copyright 2023-2025 hrzlgnm
// SPDX-License-Identifier: MIT-0

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
    constexpr auto scope {Scope::Global};
    std::array<uint8_t, 4> someIpV4 {192, 168, 17, 1};
    std::array<uint8_t, 4> someBroadcast {192, 168, 17, 255};

    const auto someV4 = ip::Address(someIpV4);
    const auto someBroadcastV4 = ip::Address(someBroadcast);
    const auto someV6 = ip::Address::fromString("2001:db8::1");
    const Address addrV6 {someV6, std::nullopt, 24, scope, AddressFlags(5), AddressAssignmentProtocol::Unspecified};
    const Address addrV4 {
        someV4, someBroadcastV4, 24, scope, AddressFlags(10), AddressAssignmentProtocol::KernelLinkLocal};
    const Address defaultAddress {};

    TEST_CASE("family")
    {
        CHECK(addrV4.family() == someV4.family());
        CHECK(defaultAddress.family() == network::Family::IPv4);
    }

    TEST_CASE("isV4")
    {
        CHECK(addrV4.isV4());
        CHECK(defaultAddress.isV4());
    }

    TEST_CASE("isV6")
    {
        CHECK(!addrV4.isV6());
        CHECK(addrV6.isV6());
        CHECK(!defaultAddress.isV6());
    }

    TEST_CASE("ip")
    {
        CHECK(addrV4.ip() == someV4);
        CHECK(defaultAddress.ip() == ip::Address {});
    }

    TEST_CASE("broadcast")
    {
        CHECK(addrV4.broadcast() == someBroadcastV4);
        CHECK(defaultAddress.broadcast() == std::nullopt);
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
        INFO("Address flags ", addrV4.flags().toString());
        CHECK(addrV4.flags().test(AddressFlag::HomeAddress));
        CHECK(addrV4.flags().test(AddressFlag::NoDuplicateAddressDetection));
        CHECK(defaultAddress.flags().none());
    }

    TEST_CASE("proto")
    {
        CHECK(addrV4.addressAssignmentProtocol() == AddressAssignmentProtocol::KernelLinkLocal);
        CHECK(addrV6.addressAssignmentProtocol() == AddressAssignmentProtocol::Unspecified);
        CHECK(defaultAddress.addressAssignmentProtocol() == AddressAssignmentProtocol::Unspecified);
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
}  // namespace
