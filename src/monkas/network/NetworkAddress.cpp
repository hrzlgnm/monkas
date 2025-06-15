#include <network/NetworkAddress.hpp>

#include <linux/rtnetlink.h>
#include <ostream>

namespace monkas::network
{

NetworkAddress::NetworkAddress(AddressFamily addressFamily, const ip::Address &address, const ip::Address &broadcast,
                               uint8_t prefixLen, AddressScope scope, uint32_t flags)
    : m_af{addressFamily}
    , m_ip{address}
    , m_brd{broadcast}
    , m_prefixlen{prefixLen}
    , m_scope{scope}
    , m_flags{flags}
{
}

NetworkAddress::operator bool() const
{
    return m_af != AddressFamily::Unspecified;
}

auto NetworkAddress::addressFamily() const -> AddressFamily
{
    return m_af;
}

auto NetworkAddress::ip() const -> const ip::Address &
{
    return m_ip;
}

auto NetworkAddress::broadcast() const -> const ip::Address &
{
    return m_brd;
}

auto NetworkAddress::prefixLength() const -> uint8_t
{
    return m_prefixlen;
}

auto NetworkAddress::scope() const -> AddressScope
{
    return m_scope;
}

auto NetworkAddress::flags() const -> uint32_t
{
    return m_flags;
}

auto fromRtnlScope(uint8_t rtnlScope) -> AddressScope
{
    switch (rtnlScope)
    {
    case RT_SCOPE_SITE:
        return AddressScope::Site;
    case RT_SCOPE_LINK:
        return AddressScope::Link;
    case RT_SCOPE_HOST:
        return AddressScope::Host;
    case RT_SCOPE_NOWHERE:
        return AddressScope::Nowhere;
    case RT_SCOPE_UNIVERSE:
    default:
        return AddressScope::Global;
    }
}

auto operator<<(std::ostream &o, AddressScope a) -> std::ostream &
{
    switch (a)
    {
    case AddressScope::Site:
        o << "site";
        break;
    case AddressScope::Link:
        o << "link";
        break;
    case AddressScope::Host:
        o << "host";
        break;
    case AddressScope::Nowhere:
        o << "nowhere";
        break;
    case AddressScope::Global:
    default:
        o << "global";
        break;
    }
    return o;
}

auto operator<<(std::ostream &o, const NetworkAddress &a) -> std::ostream &
{
    o << a.addressFamily() << " " << a.ip() << "/" << static_cast<int>(a.prefixLength());
    o << " scope " << a.scope();
    if (a.broadcast())
    {
        o << " brd " << a.broadcast();
    }
    // TODO: to readable
    if (a.flags() != 0U)
    {
        o << " f " << std::hex << a.flags() << std::dec;
    }
    return o;
}

auto operator==(const NetworkAddress &lhs, const NetworkAddress &rhs) -> bool
{
    return lhs.addressFamily() == rhs.addressFamily() && lhs.ip() == rhs.ip() && lhs.broadcast() == rhs.broadcast() &&
           lhs.prefixLength() == rhs.prefixLength() && lhs.scope() == rhs.scope() && lhs.flags() == rhs.flags();
}

auto operator!=(const NetworkAddress &lhs, const NetworkAddress &rhs) -> bool
{
    return !(lhs == rhs);
}

auto operator<(const NetworkAddress &lhs, const NetworkAddress &rhs) -> bool
{
    return lhs.ip() < rhs.ip();
}

auto operator>=(const NetworkAddress &lhs, const NetworkAddress &rhs) -> bool
{
    return !(lhs.ip() < rhs.ip());
}

} // namespace monkas::network
