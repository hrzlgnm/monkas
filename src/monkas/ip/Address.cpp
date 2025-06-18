#include <cstring>
#include <ip/Address.hpp>

#include <algorithm>
#include <arpa/inet.h>
#include <ostream>

namespace monkas::ip
{
namespace
{
constexpr auto V4_MAPPED_PREFIX = std::array<uint8_t, 12>{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff};

auto v4MappedCompare(const Address &v6, const Address &v4) noexcept -> int
{
    return std::memcmp(v6.data() + V4_MAPPED_PREFIX.size(), v4.data(), v4.addressLength());
}
} // namespace

auto asLinuxAf(Family f) -> int
{
    switch (f)
    {
    case Family::IPv4:
        return AF_INET;
    case Family::IPv6:
        return AF_INET6;
    case Family::Unspecified:
    default:
        return AF_UNSPEC;
    }
}

auto operator<<(std::ostream &o, Family a) -> std::ostream &
{
    switch (a)
    {
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

Address::Address()
    : std::array<uint8_t, IPV6_ADDR_LEN>{}
{
}

Address::operator bool() const
{
    return m_family != Family::Unspecified;
}

auto Address::family() const -> Family
{
    return m_family;
}

auto Address::addressLength() const -> Address::size_type
{
    if (m_family == Family::IPv4)
    {
        return IPV4_ADDR_LEN;
    }
    if (m_family == Family::IPv6)
    {
        return IPV6_ADDR_LEN;
    }
    return 0;
}

auto Address::isMappedV4() const -> bool
{
    if (m_family == Family::IPv6)
    {
        return std::memcmp(data(), V4_MAPPED_PREFIX.data(), V4_MAPPED_PREFIX.size()) == 0;
    }
    return false;
}

auto Address::toString() const -> std::string
{
    std::array<char, INET6_ADDRSTRLEN> out{};
    if (inet_ntop(asLinuxAf(m_family), data(), out.data(), out.size()) != nullptr)
    {
        return {out.data()};
    }
    return {"Unspecified"};
}

auto Address::fromString(const std::string &address) -> Address
{
    std::array<uint8_t, IPV6_ADDR_LEN> addr{};
    if (inet_pton(AF_INET, address.data(), addr.data()) == 1)
    {
        return Address::fromBytes(addr.data(), IPV4_ADDR_LEN);
    }
    if (inet_pton(AF_INET6, address.data(), addr.data()) == 1)
    {
        return Address::fromBytes(addr.data(), IPV6_ADDR_LEN);
    }
    return {};
}

auto Address::fromBytes(const uint8_t *bytes, size_type len) -> Address
{
    if (len == IPV6_ADDR_LEN || len == IPV4_ADDR_LEN)
    {
        Address a;
        std::copy_n(bytes, len, a.begin());
        a.m_family = (len == IPV6_ADDR_LEN ? Family::IPv6 : Family::IPv4);
        return a;
    }
    return {};
}

auto Address::fromBytes(const std::array<uint8_t, IPV4_ADDR_LEN> &bytes) -> Address
{
    return fromBytes(bytes.data(), bytes.size());
}

auto Address::fromBytes(const std::array<uint8_t, IPV6_ADDR_LEN> &bytes) -> Address
{
    return fromBytes(bytes.data(), bytes.size());
}

auto Address::operator<=>(const Address &rhs) const noexcept -> std::strong_ordering
{
    if (m_family == rhs.m_family)
    {
        const auto len = addressLength();
        if (len == 0)
        {
            return std::strong_ordering::equal;
        }
        return std::memcmp(data(), rhs.data(), len) <=> 0;
    }

    if (isMappedV4() && rhs.isV4())
    {
        return v4MappedCompare(*this, rhs) <=> 0;
    }

    if (rhs.isMappedV4() && isV4())
    {
        return 0 <=> v4MappedCompare(rhs, *this);
    }

    return m_family <=> rhs.m_family;
}

auto Address::operator==(const Address &rhs) const noexcept -> bool
{
    return (*this <=> rhs) == std::strong_ordering::equal;
}

auto operator<<(std::ostream &o, const Address &a) -> std::ostream &
{
    std::array<char, INET6_ADDRSTRLEN> buf{};
    if (inet_ntop(asLinuxAf(a.family()), a.data(), buf.data(), buf.size()) != nullptr)
    {
        o << buf.data();
    }
    else
    {
        o << "Unspecified";
    }
    return o;
}

} // namespace monkas::ip
