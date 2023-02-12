#include "Address.h"

#include <algorithm>
#include <arpa/inet.h>
#include <iostream>

namespace monkas
{
namespace ip
{
inline int as_inet_af(AddressFamily f)
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

std::ostream &operator<<(std::ostream &o, AddressFamily a)
{
    switch (a)
    {
    case AddressFamily::IPv4:
        o << "inet";
        break;
    case AddressFamily::IPv6:
        o << "ine6";
        break;
    case AddressFamily::Unspecified:
        o << "unspec";
        break;
    }
    return o;
}

Address::operator bool() const
{
    return m_af != AddressFamily::Unspecified;
}

AddressFamily Address::adressFamily() const
{
    return m_af;
}

Address Address::fromBytes(const uint8_t *bytes, size_type len)
{
    if (len == IPV6_ADDR_LEN || len == IPV4_ADDR_LEN)
    {
        Address a;
        std::copy_n(bytes, len, a.begin());
        a.m_af = (len == IPV6_ADDR_LEN ? AddressFamily::IPv6 : AddressFamily::IPv4);
        return a;
    }
    return Address();
}

std::string Address::toString() const
{
    char out[INET6_ADDRSTRLEN];
    if (inet_ntop(as_inet_af(m_af), data(), out, sizeof(out)))
    {
        return std::string(out);
    }
    return std::string();
}

std::ostream &operator<<(std::ostream &o, const Address &a)
{
    return o << a.toString();
}

} // namespace ip
} // namespace monkas
