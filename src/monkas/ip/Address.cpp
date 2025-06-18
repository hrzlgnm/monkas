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

auto Address::isMulticast() const -> bool
{
    if (m_family == Family::IPv4)
    {
        constexpr auto V4_MCAST_START = 224;
        constexpr auto V4_MCAST_END = 239;
        return data()[0] >= V4_MCAST_START && data()[0] <= V4_MCAST_END;
    }
    if (m_family == Family::IPv6)
    {
        constexpr auto V6_MCAST_PREFIX = 0xffU;
        constexpr auto V6_MCAST_MASK = 0xf0U;
        return data()[0] == V6_MCAST_PREFIX && (data()[1] & V6_MCAST_MASK) == 0x00;
    }
    return false;
}

auto Address::isLinkLocal() const -> bool
{
    if (m_family == Family::IPv4)
    {
        constexpr auto LINK_LOCAL_START = 169;
        constexpr auto LINK_LOCAL_END = 254;
        return data()[0] == LINK_LOCAL_START && data()[1] == LINK_LOCAL_END;
    }
    if (m_family == Family::IPv6)
    {
        constexpr auto V6_LL_PREFIX = 0xfeU;
        constexpr auto V6_LL_MASK = 0xc0U;
        constexpr auto V6_LL_BITS = 0x80U;
        return data()[0] == V6_LL_PREFIX && (data()[1] & V6_LL_MASK) == V6_LL_BITS;
    }
    return false;
}

auto Address::isUniqueLocal() const -> bool
{
    if (m_family == Family::IPv6)
    {
        // Unique local IPv6 addresses are in fc00::/7 (first 7 bits are 1111 110)
        // So, check if the first byte & 0xFE == 0xFC
        constexpr auto V6_UL_PREFIX = 0xFCU;
        constexpr auto V6_UL_MASK = 0xFEU;
        return (data()[0] & V6_UL_MASK) == V6_UL_PREFIX;
    }
    return false;
}

auto Address::isLoopback() const -> bool
{
    // Loopback address in IPv4 is
    constexpr uint8_t LOOPBACK_FIRST_OCTET = 127;
    if (isV4())
    {
        return data()[0] == LOOPBACK_FIRST_OCTET;
    }
    if (isMappedV4())
    {
        return data()[V4_MAPPED_PREFIX.size()] == LOOPBACK_FIRST_OCTET;
    }
    if (isV6())
    {
        // Loopback address in IPv6 is ::1
        return std::count(cbegin(), cend(), 0) == IPV6_ADDR_LEN - 1 && data()[IPV6_ADDR_LEN - 1] == 1;
    }
    return false;
}

auto Address::isBroadcast() const -> bool
{
    if (m_family == Family::IPv4)
    {
        constexpr uint8_t IPV4_BROADCAST_BYTE = 0xFF;
        return std::all_of(cbegin(), cbegin() + addressLength(),
                           [](uint8_t byte) { return byte == IPV4_BROADCAST_BYTE; });
    }
    return false;
}

auto Address::isPrivate() const -> bool
{
    // Private IPv4 ranges:
    // 10.0.0.0/8:   10.0.0.0 - 10.255.255.255
    // 172.16.0.0/12: 172.16.0.0 - 172.31.255.255
    // 192.168.0.0/16: 192.168.0.0 - 192.168.255.255
    constexpr uint32_t IPV4_10_0_0_0 = 0x0A000000;        // 10.0.0.0
    constexpr uint32_t IPV4_10_255_255_255 = 0x0AFFFFFF;  // 10.255.255.255
    constexpr uint32_t IPV4_172_16_0_0 = 0xAC100000;      // 172.16.0.0
    constexpr uint32_t IPV4_172_31_255_255 = 0xAC1FFFFF;  // 172.31.255.255
    constexpr uint32_t IPV4_192_168_0_0 = 0xC0A80000;     // 192.168.0.0
    constexpr uint32_t IPV4_192_168_255_255 = 0xC0A8FFFF; // 192.168.255.255

    if (isV4())
    {
        uint32_t addr = ntohl(*std::bit_cast<uint32_t *>(data()));
        if ((addr >= IPV4_10_0_0_0 && addr <= IPV4_10_255_255_255) ||   // 10.0.0.0/8
            (addr >= IPV4_172_16_0_0 && addr <= IPV4_172_31_255_255) || // 172.16.0.0/12
            (addr >= IPV4_192_168_0_0 && addr <= IPV4_192_168_255_255)) // 192.168.0.0/16
        {
            return true;
        }
    }
    if (isMappedV4())
    {
        // Check the mapped IPv4 address
        uint32_t addr = ntohl(*std::bit_cast<uint32_t *>(data() + V4_MAPPED_PREFIX.size()));
        if ((addr >= IPV4_10_0_0_0 && addr <= IPV4_10_255_255_255) ||   // 10.0.0.0/8
            (addr >= IPV4_172_16_0_0 && addr <= IPV4_172_31_255_255) || // 172.16.0.0/12
            (addr >= IPV4_192_168_0_0 && addr <= IPV4_192_168_255_255)) // 192.168.0.0/16
        {
            return true;
        }
    }
    if (isV6())
    {
        return isUniqueLocal(); // Unique local addresses in IPv6 are considered private
    }
    return false;
}

auto Address::isDocumentation() const -> bool
{
    if (!isV4() && !isMappedV4())
    {
        return false; // Only IPv4 addresses can be documentation addresses
    }

    // Documentation IPv4 ranges according to RFC 5737:
    // 192.0.2.0/24
    // 198.51.100.0/24
    // 203.0.113.0/24
    static constexpr uint32_t DOC1_BASE = (192U << 24U) | (0U << 16U) | (2U << 8U);    // 192.0.2.0
    static constexpr uint32_t DOC2_BASE = (198U << 24U) | (51U << 16U) | (100U << 8U); // 198.51.100.0
    static constexpr uint32_t DOC3_BASE = (203U << 24U) | (0U << 16U) | (113U << 8U);  // 203.0.113.0
    static constexpr uint32_t DOC_MASK = 0xFFFFFF00;                                   // /24 mask

    // Assume Address has a method to get IPv4 as uint32_t in host byte order
    uint32_t addr = ntohl(*std::bit_cast<uint32_t *>(isMappedV4() ? data() + V4_MAPPED_PREFIX.size() : data()));

    return ((addr & DOC_MASK) == DOC1_BASE) || ((addr & DOC_MASK) == DOC2_BASE) || ((addr & DOC_MASK) == DOC3_BASE);
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

auto Address::toMappedV4() const -> std::optional<Address>
{
    if (isV4())
    {
        Address v6;
        // Set the IPv4-mapped IPv6 prefix: ::ffff:0:0/96
        constexpr size_t PREFIX_FF_INDEX_1 = 10;
        constexpr size_t PREFIX_FF_INDEX_2 = 11;
        constexpr uint8_t PREFIX_FF_VALUE = 0xff;
        v6[PREFIX_FF_INDEX_1] = PREFIX_FF_VALUE;
        v6[PREFIX_FF_INDEX_2] = PREFIX_FF_VALUE;
        std::copy_n(data(), IPV4_ADDR_LEN, v6.begin() + V4_MAPPED_PREFIX.size());
        v6.m_family = Family::IPv6;
        return v6;
    }
    return std::nullopt;
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
