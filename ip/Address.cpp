#include "Address.h"

#include <algorithm>
#include <arpa/inet.h>
#include <iostream>

namespace monkas
{
namespace ip
{
int asLinuxAf(AddressFamily f)
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
        o << "inet6";
        break;
    case AddressFamily::Unspecified:
        o << "unspec";
        break;
    }
    return o;
}

Address::operator bool() const
{
    return m_addressFamily != AddressFamily::Unspecified;
}

AddressFamily Address::adressFamily() const
{
    return m_addressFamily;
}

Address::size_type Address::addressLength() const
{
    if (m_addressFamily == AddressFamily::IPv4)
        return IPV4_ADDR_LEN;
    if (m_addressFamily == AddressFamily::IPv6)
        return IPV6_ADDR_LEN;
    return 0;
}

Address Address::fromBytes(const uint8_t *bytes, size_type len)
{
    if (len == IPV6_ADDR_LEN || len == IPV4_ADDR_LEN)
    {
        Address a;
        std::copy_n(bytes, len, a.begin());
        a.m_addressFamily = (len == IPV6_ADDR_LEN ? AddressFamily::IPv6 : AddressFamily::IPv4);
        return a;
    }
    return Address();
}

std::string Address::toString() const
{
    char out[INET6_ADDRSTRLEN];
    if (inet_ntop(asLinuxAf(m_addressFamily), data(), out, sizeof(out)))
    {
        return std::string(out);
    }
    return std::string();
}

std::ostream &operator<<(std::ostream &o, const Address &a)
{
    return o << a.toString();
}

bool operator<(const Address &lhs, const Address &rhs)
{
    if (lhs.adressFamily() == rhs.adressFamily())
    {
        const auto addressLenght = lhs.addressLength();
        return std::lexicographical_compare(lhs.begin(), lhs.begin() + addressLenght, rhs.begin(),
                                            rhs.begin() + addressLenght);
    }
    return lhs.addressLength() < rhs.addressLength();
}

} // namespace ip
} // namespace monkas
