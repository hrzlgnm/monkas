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

auto v4MappedCompare(const Address &v6, const Address &v4) noexcept -> int
{

    return std::memcmp(v6.data() + v4MappedPrefix.size(), v4.data(), v4.addressLength());
}
} // namespace

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

auto operator<(const Address &lhs, const Address &rhs) noexcept -> bool
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

auto operator<=(const Address &lhs, const Address &rhs) noexcept -> bool
{
    return !(rhs < lhs);
}

auto operator>=(const Address &lhs, const Address &rhs) noexcept -> bool
{
    return !(lhs < rhs);
}

auto operator>(const Address &lhs, const Address &rhs) noexcept -> bool
{
    return !(lhs <= rhs);
}

auto operator==(const Address &lhs, const Address &rhs) noexcept -> bool
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

auto operator!=(const Address &lhs, const Address &rhs) noexcept -> bool
{
    return !(lhs == rhs);
}

} // namespace monkas::ip
