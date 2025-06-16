#include <cassert>
#include <cstring>
#include <ip/Address.hpp>

#include <algorithm>
#include <arpa/inet.h>
#include <ostream>

namespace monkas::ip
{
namespace
{
constexpr auto v4MappedPrefix = std::array<uint8_t, 12>{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff};

/**
 * @brief Compares the IPv4 portion of an IPv4-mapped IPv6 address with a pure IPv4 address.
 *
 * Asserts that the first address is IPv4-mapped and the second is IPv4. Returns the result of comparing the embedded IPv4 bytes in the IPv6 address to the IPv4 address bytes.
 *
 * @return int Negative if v6's IPv4 portion is less than v4, zero if equal, positive if greater.
 */
auto v4MappedCompare(const Address &v6, const Address &v4) -> int
{
    assert(v6.isMappedV4() && v4.addressFamily() == AddressFamily::IPv4);
    return std::memcmp(v6.data() + v4MappedPrefix.size(), v4.data(), v4.addressLength());
}
} /**
 * @brief Converts an AddressFamily value to the corresponding Linux address family constant.
 *
 * @param f The address family to convert.
 * @return int The Linux address family constant (AF_INET, AF_INET6, or AF_UNSPEC).
 */

auto asLinuxAf(AddressFamily f) -> int
{
    switch (f)
    {
    case AddressFamily::IPv4:
        return AF_INET;
    case AddressFamily::IPv6:
        return AF_INET6;
    case AddressFamily::Unspecified:
    default:
        return AF_UNSPEC;
    }
}

auto operator<<(std::ostream &o, AddressFamily a) -> std::ostream &
{
    switch (a)
    {
    case AddressFamily::IPv4:
        o << "inet";
        break;
    case AddressFamily::IPv6:
        o << "inet6";
        break;
    case AddressFamily::Unspecified:
        o << "unspec";
        break;
    }
    return o;
}

Address::Address()
    : std::array<uint8_t, ipV6AddrLen>{}
{
}

Address::operator bool() const
{
    return m_addressFamily != AddressFamily::Unspecified;
}

auto Address::addressFamily() const -> AddressFamily
{
    return m_addressFamily;
}

auto Address::addressLength() const -> Address::size_type
{
    if (m_addressFamily == AddressFamily::IPv4)
    {
        return ipV4AddrLen;
    }
    if (m_addressFamily == AddressFamily::IPv6)
    {
        return ipV6AddrLen;
    }
    return 0;
}

auto Address::isMappedV4() const -> bool
{
    if (m_addressFamily == AddressFamily::IPv6)
    {
        return std::memcmp(data(), v4MappedPrefix.data(), v4MappedPrefix.size()) == 0;
    }
    return false;
}

auto Address::toString() const -> std::string
{
    std::array<char, INET6_ADDRSTRLEN> out{};
    if (inet_ntop(asLinuxAf(m_addressFamily), data(), out.data(), out.size()) != nullptr)
    {
        return {out.data()};
    }
    return {"Unspecified"};
}

auto Address::fromString(const std::string &address) -> Address
{
    std::array<uint8_t, ipV6AddrLen> addr{};
    if (inet_pton(AF_INET, address.data(), addr.data()) == 1)
    {
        return Address::fromBytes(addr.data(), ipV4AddrLen);
    }
    if (inet_pton(AF_INET6, address.data(), addr.data()) == 1)
    {
        return Address::fromBytes(addr.data(), ipV6AddrLen);
    }
    return {};
}

auto Address::fromBytes(const uint8_t *bytes, size_type len) -> Address
{
    if (len == ipV6AddrLen || len == ipV4AddrLen)
    {
        Address a;
        std::copy_n(bytes, len, a.begin());
        a.m_addressFamily = (len == ipV6AddrLen ? AddressFamily::IPv6 : AddressFamily::IPv4);
        return a;
    }
    return {};
}

auto Address::fromBytes(const std::array<uint8_t, ipV4AddrLen> &bytes) -> Address
{
    return fromBytes(bytes.data(), bytes.size());
}

auto Address::fromBytes(const std::array<uint8_t, ipV6AddrLen> &bytes) -> Address
{
    return fromBytes(bytes.data(), bytes.size());
}

auto operator<<(std::ostream &o, const Address &a) -> std::ostream &
{
    return o << a.toString();
}

/**
 * @brief Compares two IP addresses for ordering.
 *
 * Determines if the left-hand address is less than the right-hand address, considering address family, mapped IPv4 addresses, and raw byte values. IPv4-mapped IPv6 addresses are compared to IPv4 addresses by their embedded IPv4 portion. If address families differ and neither is a mapped IPv4, comparison is based on address length.
 *
 * @return true if lhs is less than rhs; otherwise, false.
 */
auto operator<(const Address &lhs, const Address &rhs) -> bool
{
    if (lhs.addressFamily() == rhs.addressFamily())
    {
        const auto addressLength = lhs.addressLength();
        return std::memcmp(lhs.data(), rhs.data(), addressLength) < 0;
    }
    if (lhs.isMappedV4() && rhs.addressFamily() == AddressFamily::IPv4)
    {
        return v4MappedCompare(lhs, rhs) < 0;
    }
    if (rhs.isMappedV4() && lhs.addressFamily() == AddressFamily::IPv4)
    {
        return v4MappedCompare(rhs, lhs) < 0;
    }
    return lhs.addressLength() < rhs.addressLength();
}

/**
 * @brief Determines if one IP address is less than or equal to another.
 *
 * Compares two Address instances for ordering, returning true if the left-hand side is less than or equal to the right-hand side.
 *
 * @return true if lhs is less than or equal to rhs; otherwise, false.
 */
auto operator<=(const Address &lhs, const Address &rhs) -> bool
{
    return !(rhs < lhs);
}

/**
 * @brief Determines if one IP address is greater than or equal to another.
 *
 * Returns true if the left-hand address is greater than or equal to the right-hand address according to the defined ordering for Address objects.
 *
 * @return true if lhs is greater than or equal to rhs; false otherwise.
 */
auto operator>=(const Address &lhs, const Address &rhs) -> bool
{
    return !(lhs < rhs);
}

/**
 * @brief Determines if one IP address is greater than another.
 *
 * Compares two Address instances and returns true if the left-hand side is greater than the right-hand side according to address family and byte order.
 *
 * @return true if lhs is greater than rhs; false otherwise.
 */
auto operator>(const Address &lhs, const Address &rhs) -> bool
{
    return !(lhs <= rhs);
}

/**
 * @brief Checks if two IP addresses are equal.
 *
 * Compares two Address instances for equality, treating IPv4-mapped IPv6 addresses and pure IPv4 addresses as equal if their IPv4 portions match.
 *
 * @return true if the addresses are equal, false otherwise.
 */
auto operator==(const Address &lhs, const Address &rhs) -> bool
{
    if (lhs.addressFamily() == rhs.addressFamily())
    {
        const auto addressLength = lhs.addressLength();
        return std::memcmp(lhs.data(), rhs.data(), addressLength) == 0;
    }
    if (lhs.isMappedV4() && rhs.addressFamily() == AddressFamily::IPv4)
    {
        return v4MappedCompare(lhs, rhs) == 0;
    }
    if (rhs.isMappedV4() && lhs.addressFamily() == AddressFamily::IPv4)
    {
        return v4MappedCompare(rhs, lhs) == 0;
    }
    return false;
}

auto operator!=(const Address &lhs, const Address &rhs) -> bool
{
    return !(lhs == rhs);
}

} // namespace monkas::ip
