#include "NetworkAddress.h"

#include <iostream>
#include <linux/rtnetlink.h>

namespace monkas
{
namespace network
{

NetworkAddress::NetworkAddress(AddressFamily addresFamily, const ip::Address &address, const ip::Address &broadcast,
                               uint8_t prefixLen, AddressScope scope, uint32_t flags)
    : m_addressFamily{addresFamily}, m_ip{address}, m_broadcast{broadcast},
      m_prefixLenght{prefixLen}, m_scope{scope}, m_flags{flags}
{
}

NetworkAddress::operator bool() const
{
    return m_addressFamily != AddressFamily::Unspecified;
}

AddressFamily NetworkAddress::adressFamily() const
{
    return m_addressFamily;
}

const ip::Address &NetworkAddress::ip() const
{
    return m_ip;
}

const ip::Address &NetworkAddress::broadcast() const
{
    return m_broadcast;
}

uint8_t NetworkAddress::prefixLenght() const
{
    return m_prefixLenght;
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
        return AddressScope::NoWhere;
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
    case AddressScope::NoWhere:
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
    o << a.adressFamily() << " " << a.ip() << "/" << static_cast<int>(a.prefixLenght());
    o << " scope " << a.scope();
    if (a.broadcast())
    {
        o << " brd " << a.broadcast();
    }
    return o;
}

} // namespace network
} // namespace monkas
