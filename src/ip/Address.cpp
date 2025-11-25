#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <utility>
#include <variant>

#include <arpa/inet.h>
#include <ip/Address.hpp>
#include <overloaded/Overloaded.hpp>

namespace monkas::ip
{
auto asLinuxAf(const Family f) -> int
{
    using enum Family;
    switch (f) {
        case IPv4:
            return AF_INET;
        case IPv6:
            return AF_INET6;
        default:
            return AF_UNSPEC;
    }
}

auto operator<<(std::ostream& o, const Family f) -> std::ostream&
{
    using enum Family;
    switch (f) {
        case IPv4:
            o << "inet";
            break;
        case IPv6:
            o << "inet6";
            break;
        default:
            o << "unspec";
            break;
    }
    return o;
}

Address::Address() = default;

Address::Address(const V4Bytes& bytes)
    : m_bytes(bytes)
{
}

Address::Address(const V6Bytes& bytes)
    : m_bytes(bytes)
{
}

auto Address::isV4() const -> bool
{
    return std::holds_alternative<V4Bytes>(m_bytes);
}

auto Address::isV6() const -> bool
{
    return std::holds_alternative<V6Bytes>(m_bytes);
}

auto Address::isMulticast() const -> bool
{
    return std::visit(Overloaded {[](const V4Bytes& addr) -> bool
                                  {
                                      // IPv4 multicast addresses are in the range
                                      constexpr auto V4_MCAST_START = 224;
                                      constexpr auto V4_MCAST_END = 239;
                                      const auto upperOctet = addr[0];
                                      return upperOctet >= V4_MCAST_START && upperOctet <= V4_MCAST_END;
                                  },
                                  [](const V6Bytes& addr) -> bool
                                  {
                                      // IPv6 multicast addresses start with 0xff
                                      constexpr auto V6_MCAST_PREFIX = 0xffU;
                                      return addr[0] == V6_MCAST_PREFIX;
                                  }},
                      m_bytes);
}

auto Address::isUnicastLinkLocal() const -> bool
{
    return std::visit(Overloaded {[](const V4Bytes& addr) -> bool
                                  {
                                      constexpr auto LINK_LOCAL_START = 169;
                                      constexpr auto LINK_LOCAL_END = 254;
                                      return addr[0] == LINK_LOCAL_START && addr[1] == LINK_LOCAL_END;
                                  },
                                  [](const V6Bytes& addr) -> bool
                                  {
                                      constexpr auto V6_LL_PREFIX = 0xfeU;
                                      constexpr auto V6_LL_MASK = 0xc0U;
                                      constexpr auto V6_LL_BITS = 0x80U;
                                      return addr[0] == V6_LL_PREFIX && (addr[1] & V6_LL_MASK) == V6_LL_BITS;
                                  }},
                      m_bytes);
}

auto Address::isUniqueLocal() const -> bool
{
    if (!isV6()) {
        return false;
    }
    const auto bytes = std::get<V6Bytes>(m_bytes);

    // Unique local IPv6 addresses are in fc00::/7 (first 7 bits are 1111 110)
    // So, check if the first byte & 0xFE == 0xFC
    constexpr auto V6_UL_PREFIX = 0xFCU;
    constexpr auto V6_UL_MASK = 0xFEU;
    return (bytes[0] & V6_UL_MASK) == V6_UL_PREFIX;
}

auto Address::isLoopback() const -> bool
{
    return std::visit(Overloaded {[](const V4Bytes& addr) -> bool
                                  {
                                      // Loopback address in IPv4 is
                                      constexpr uint8_t LOOPBACK_FIRST_OCTET = 127;
                                      return addr[0] == LOOPBACK_FIRST_OCTET;
                                  },
                                  [](const V6Bytes& addr) -> bool
                                  {
                                      // Loopback address in IPv6 is ::1
                                      return std::all_of(
                                                 addr.cbegin(), addr.cend() - 1, [](const uint8_t b) { return b == 0; })
                                          && addr[IPV6_ADDR_LEN - 1] == 1;
                                  }},
                      m_bytes);
}

auto Address::isBroadcast() const -> bool
{
    if (!isV4()) {
        return false;
    }
    auto bytes = std::get<V4Bytes>(m_bytes);
    constexpr uint8_t IPV4_BROADCAST_BYTE = 0xFF;
    return std::ranges::all_of(std::as_const(bytes), [](const uint8_t byte) { return byte == IPV4_BROADCAST_BYTE; });
}

auto Address::family() const -> Family
{
    return std::visit(
        Overloaded {[](const V4Bytes&) { return Family::IPv4; }, [](const V6Bytes&) { return Family::IPv6; }}, m_bytes);
}

auto Address::toString() const -> std::string
{
    return std::visit(Overloaded {[](const V4Bytes& addr) -> std::string
                                  {
                                      std::array<char, INET_ADDRSTRLEN> buffer {};
                                      inet_ntop(AF_INET, addr.data(), buffer.data(), buffer.size());
                                      return std::string(buffer.data());
                                  },
                                  [](const V6Bytes& addr) -> std::string
                                  {
                                      std::array<char, INET6_ADDRSTRLEN> buffer {};
                                      inet_ntop(AF_INET6, addr.data(), buffer.data(), buffer.size());
                                      return std::string(buffer.data());
                                  }},
                      m_bytes);
}

auto Address::fromString(const std::string& address) noexcept(false) -> Address
{
    {
        V4Bytes addr {};
        if (inet_pton(AF_INET, address.data(), addr.data()) == 1) {
            return Address(addr);
        }
    }
    {
        V6Bytes addr {};
        if (inet_pton(AF_INET6, address.data(), addr.data()) == 1) {
            return Address(addr);
        }
    }
    throw std::invalid_argument("Failed to parse address '" + address
                                + "': Invalid format or unsupported address family");
}

auto Address::operator<=>(const Address& rhs) const -> std::strong_ordering
{
    return m_bytes <=> rhs.m_bytes;
}

auto operator<<(std::ostream& o, const Address& a) -> std::ostream&
{
    return o << a.toString();
}

}  // namespace monkas::ip
