#include <algorithm>
#include <cstring>
#include <ostream>
#include <stdexcept>
#include <utility>
#include <variant>

#include <arpa/inet.h>
#include <ip/Address.hpp>
#include <overloaded/Overloaded.hpp>

namespace monkas::ip
{
/**
 * @brief Converts an address family enum to the corresponding Linux address family constant.
 *
 * @param f The address family (IPv4 or IPv6).
 * @return int The Linux address family constant (`AF_INET`, `AF_INET6`, or `AF_UNSPEC`).
 */
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

/**
 * @brief Outputs a string representation of the IP address family to a stream.
 *
 * Writes "inet" for IPv4, "inet6" for IPv6, and "unspec" for any other value.
 *
 * @return Reference to the output stream.
 */
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

Address::Address(const IpV4Bytes& bytes)
    : m_bytes(bytes)
{
}

/**
 * @brief Constructs an Address from an IPv6 byte array.
 *
 * Initializes the Address to represent the specified IPv6 address.
 */
Address::Address(const IpV6Bytes& bytes)
    : m_bytes(bytes)
{
}

/**
 * @brief Checks if the address is an IPv4 address.
 *
 * @return true if the address is stored as IPv4, false otherwise.
 */
auto Address::isV4() const -> bool
{
    return std::holds_alternative<IpV4Bytes>(m_bytes);
}

/**
 * @brief Checks if the address is an IPv6 address.
 *
 * @return true if the address is stored as IPv6, false otherwise.
 */
auto Address::isV6() const -> bool
{
    return std::holds_alternative<IpV6Bytes>(m_bytes);
}

/**
 * @brief Determines if the address is a multicast address.
 *
 * @return true if the address is multicast (IPv4: 224.0.0.0–239.255.255.255, IPv6: addresses starting with 0xff); false otherwise.
 */
auto Address::isMulticast() const -> bool
{
    return std::visit(Overloaded {[](const IpV4Bytes& addr)
                                  {
                                      // IPv4 multicast addresses are in the range
                                      constexpr auto V4_MCAST_START = 224;
                                      constexpr auto V4_MCAST_END = 239;
                                      const auto upperOctet = addr[0];
                                      return upperOctet >= V4_MCAST_START && upperOctet <= V4_MCAST_END;
                                  },
                                  [](const IpV6Bytes& addr)
                                  {
                                      // IPv6 multicast addresses start with 0xff
                                      constexpr auto V6_MCAST_PREFIX = 0xffU;
                                      return addr[0] == V6_MCAST_PREFIX;
                                  }},
                      m_bytes);
}

/**
 * @brief Determines if the address is a link-local address.
 *
 * For IPv4, returns true if the address is in the 169.254.0.0/16 range.  
 * For IPv6, returns true if the address is in the fe80::/10 range.
 *
 * @return true if the address is link-local, false otherwise.
 */
auto Address::isLinkLocal() const -> bool
{
    return std::visit(Overloaded {[](const IpV4Bytes& addr)
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

/**
 * @brief Determines if the address is a loopback address.
 *
 * Returns true if the address is the IPv4 loopback (127.0.0.0/8) or the IPv6 loopback (::1).
 *
 * @return true if the address is a loopback address, false otherwise.
 */
auto Address::isLoopback() const -> bool
{
    return std::visit(Overloaded {[](const IpV4Bytes& addr)
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
    return std::ranges::all_of(std::as_const(bytes), [](const uint8_t byte) { return byte == IPV4_BROADCAST_BYTE; });
}

/**
 * @brief Returns the address family (IPv4 or IPv6) of the stored address.
 *
 * @return Family The address family corresponding to the stored address type.
 */
auto Address::family() const -> Family
{
    return std::visit(
        Overloaded {[](const IpV4Bytes&) { return Family::IPv4; }, [](const IpV6Bytes&) { return Family::IPv6; }},
        m_bytes);
}

/**
 * @brief Returns the standard string representation of the IP address.
 *
 * Converts the stored IPv4 or IPv6 address to its textual form using the appropriate format.
 * @return String representation of the address (e.g., "192.168.1.1" or "2001:db8::1").
 */
auto Address::toString() const -> std::string
{
    return std::visit(Overloaded {[](const IpV4Bytes& addr)
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

/**
 * @brief Parses a string as an IPv4 or IPv6 address.
 *
 * Attempts to interpret the input string as either an IPv4 or IPv6 address. Returns an Address object on success.
 *
 * @param address The string representation of the IP address.
 * @return Address The parsed address.
 * @throws std::invalid_argument If the string cannot be parsed as a valid IPv4 or IPv6 address.
 */
auto Address::fromString(const std::string& address) noexcept(false) -> Address
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
    throw std::invalid_argument("Failed to parse address '" + address
                                + "': Invalid format or unsupported address family");
}

/**
 * @brief Compares two IP addresses for ordering.
 *
 * Performs a lexicographical comparison of the underlying byte arrays of the addresses.
 *
 * @param rhs The address to compare with.
 * @return std::strong_ordering Result of the comparison.
 */
auto Address::operator<=>(const Address& rhs) const noexcept -> std::strong_ordering
{
    return m_bytes <=> rhs.m_bytes;
}

auto operator<<(std::ostream& o, const Address& a) -> std::ostream&
{
    return o << a.toString();
}

}  // namespace monkas::ip
