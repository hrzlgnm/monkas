#include <network/NetworkAddress.hpp>

#include <iostream>
#include <linux/rtnetlink.h>

namespace monkas
{
namespace network
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

AddressFamily NetworkAddress::addressFamily() const
{
    return m_af;
}

const ip::Address &NetworkAddress::ip() const
{
    return m_ip;
}

const ip::Address &NetworkAddress::broadcast() const
{
    return m_brd;
}

uint8_t NetworkAddress::prefixLength() const
{
    return m_prefixlen;
}

AddressScope NetworkAddress::scope() const
{
    return m_scope;
}

uint32_t NetworkAddress::flags() const
{
    return m_flags;
}

AddressScope fromRtnlScope(uint8_t rtnlScope)
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

std::ostream &operator<<(std::ostream &o, AddressScope a)
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

std::ostream &operator<<(std::ostream &o, const NetworkAddress &a)
{
    o << a.addressFamily() << " " << a.ip() << "/" << static_cast<int>(a.prefixLength());
    o << " scope " << a.scope();
    if (a.broadcast())
    {
        o << " brd " << a.broadcast();
    }
    // TODO: to readable
    if (a.flags())
    {
        o << " f " << std::hex << a.flags() << std::dec;
    }
    return o;
}

bool operator==(const NetworkAddress &lhs, const NetworkAddress &rhs)
{
    return lhs.addressFamily() == rhs.addressFamily() && lhs.ip() == rhs.ip() && lhs.broadcast() == rhs.broadcast() &&
           lhs.prefixLength() == rhs.prefixLength() && lhs.scope() == rhs.scope() && lhs.flags() == rhs.flags();
}

bool operator!=(const NetworkAddress &lhs, const NetworkAddress &rhs)
{
    return !(lhs == rhs);
}

bool operator<(const NetworkAddress &lhs, const NetworkAddress &rhs)
{
    return lhs.ip() < rhs.ip();
}

bool operator>=(const NetworkAddress &lhs, const NetworkAddress &rhs)
{
    return !(lhs.ip() < rhs.ip());
}

} // namespace network
} // namespace monkas
