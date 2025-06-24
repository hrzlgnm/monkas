#include <algorithm>
#include <cstring>
#include <ostream>
#include <variant>

#include <arpa/inet.h>
#include <ip/Address.hpp>

namespace monkas::ip
{
namespace
{
// helper type for std::visit
template<typename... T>
struct Overloaded : T...
{
    using T::operator()...;
};
template<class... T>
Overloaded(T...) -> Overloaded<T...>;

}  // namespace

auto asLinuxAf(Family f) -> int
{
    switch (f) {
        case Family::IPv4:
            return AF_INET;
        case Family::IPv6:
            return AF_INET6;
        case Family::Unspecified:
        default:
            return AF_UNSPEC;
    }
}

auto operator<<(std::ostream& o, Family a) -> std::ostream&
{
    switch (a) {
        case Family::IPv4:
            o << "inet";
            break;
        case Family::IPv6:
            o << "inet6";
            break;
        case Family::Unspecified:
            o << "unspec";
            break;
    }
    return o;
}

Address::Address() = default;

Address::Address(const IpV4Bytes& bytes)
    : m_bytes(bytes)
{
}

Address::Address(const IpV6Bytes& bytes)
    : m_bytes(bytes)
{
}

Address::operator bool() const
{
    return !isUnspecified();
}

auto Address::isV4() const -> bool
{
    return std::holds_alternative<IpV4Bytes>(m_bytes);
}

auto Address::isV6() const -> bool
{
    return std::holds_alternative<IpV6Bytes>(m_bytes);
}

auto Address::isUnspecified() const -> bool
{
    return std::holds_alternative<std::monostate>(m_bytes);
}

auto Address::isMulticast() const -> bool
{
    return std::visit(Overloaded {[](const std::monostate&) { return false; },
                                  [](const IpV4Bytes& addr)
                                  {
                                      // IPv4 multicast addresses are in the range
                                      constexpr auto V4_MCAST_START = 224;
                                      constexpr auto V4_MCAST_END = 239;
                                      const auto upperOctet = addr[0];
                                      return upperOctet >= V4_MCAST_START && upperOctet <= V4_MCAST_END;
                                      return upperOctetd >= V4_MCAST_START && upperOctetd <= V4_MCAST_END;
                                  },
                                  [](const IpV6Bytes& addr)
                                  {
                                      // IPv6 multicast addresses start with 0xff
                                      constexpr auto V6_MCAST_PREFIX = 0xffU;
                                      return addr[0] == V6_MCAST_PREFIX;
                                  }},
                      m_bytes);
}

auto Address::isLinkLocal() const -> bool
{
    return std::visit(Overloaded {[](const std::monostate&) { return false; },
                                  [](const IpV4Bytes& addr)
                                  {
                                      constexpr auto LINK_LOCAL_START = 169;
                                      constexpr auto LINK_LOCAL_END = 254;
                                      return addr[0] == LINK_LOCAL_START && addr[1] == LINK_LOCAL_END;
                                  },
                                  [](const IpV6Bytes& addr)
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
    const auto bytes = std::get<IpV6Bytes>(m_bytes);

    // Unique local IPv6 addresses are in fc00::/7 (first 7 bits are 1111 110)
    // So, check if the first byte & 0xFE == 0xFC
    constexpr auto V6_UL_PREFIX = 0xFCU;
    constexpr auto V6_UL_MASK = 0xFEU;
    return (bytes[0] & V6_UL_MASK) == V6_UL_PREFIX;
}

auto Address::isLoopback() const -> bool
{
    return std::visit(Overloaded {[](const std::monostate&) { return false; },
                                  [](const IpV4Bytes& addr)
                                  {
                                      // Loopback address in IPv4 is
                                      constexpr uint8_t LOOPBACK_FIRST_OCTET = 127;
                                      return addr[0] == LOOPBACK_FIRST_OCTET;
                                  },
                                  [](const IpV6Bytes& addr)
                                  {
                                      // Loopback address in IPv6 is ::1
                                      return std::all_of(
                                                 addr.cbegin(), addr.cend() - 1, [](uint8_t b) { return b == 0; })
                                          && addr[IPV6_ADDR_LEN - 1] == 1;
                                  }},
                      m_bytes);
}

auto Address::isBroadcast() const -> bool
{
    if (!isV4()) {
        return false;
    }
    auto bytes = std::get<IpV4Bytes>(m_bytes);
    constexpr uint8_t IPV4_BROADCAST_BYTE = 0xFF;
    return std::all_of(bytes.cbegin(), bytes.cend(), [](uint8_t byte) { return byte == IPV4_BROADCAST_BYTE; });
}

auto Address::family() const -> Family
{
    return std::visit(Overloaded {[](const std::monostate&) { return Family::Unspecified; },
                                  [](const IpV4Bytes&) { return Family::IPv4; },
                                  [](const IpV6Bytes&) { return Family::IPv6; }},
                      m_bytes);
}

auto Address::toString() const -> std::string
{
    return std::visit(Overloaded {[](const std::monostate&) { return std::string("Unspecified"); },
                                  [](const IpV4Bytes& addr)
                                  {
                                      std::array<char, INET_ADDRSTRLEN> buffer {};
                                      inet_ntop(AF_INET, addr.data(), buffer.data(), buffer.size());
                                      return std::string(buffer.data());
                                  },
                                  [](const IpV6Bytes& addr)
                                  {
                                      std::array<char, INET6_ADDRSTRLEN> buffer {};
                                      inet_ntop(AF_INET6, addr.data(), buffer.data(), buffer.size());
                                      return std::string(buffer.data());
                                  }},
                      m_bytes);
}

auto Address::fromString(const std::string& address) -> Address
{
    {
        IpV4Bytes addr {};
        if (inet_pton(AF_INET, address.data(), addr.data()) == 1) {
            return Address(addr);
        }
    }
    {
        IpV6Bytes addr {};
        if (inet_pton(AF_INET6, address.data(), addr.data()) == 1) {
            return Address(addr);
        }
    }
    return {};
}

auto Address::operator<=>(const Address& rhs) const noexcept -> std::strong_ordering
{
    return m_bytes <=> rhs.m_bytes;
}

auto operator<<(std::ostream& o, const Address& a) -> std::ostream&
{
    return o << a.toString();
}

}  // namespace monkas::ip
