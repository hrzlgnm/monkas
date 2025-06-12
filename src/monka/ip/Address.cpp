#include <ip/Address.h>

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

Address Address::fromString(std::string_view address)
{
    std::array<uint8_t, IPV6_ADDR_LEN> addr6;
    if (inet_pton(AF_INET, address.data(), addr6.data()) == 1)
    {
        return Address::fromBytes(addr6.data(), IPV4_ADDR_LEN);
    }
    if (inet_pton(AF_INET6, address.data(), addr6.data()) == 1)
    {
        return Address::fromBytes(addr6.data(), IPV6_ADDR_LEN);
    }
    return Address();
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

Address Address::fromBytes(const std::array<uint8_t, IPV4_ADDR_LEN> &bytes)
{
    return fromBytes(bytes.data(), bytes.size());
}

Address Address::fromBytes(const std::array<uint8_t, IPV6_ADDR_LEN> &bytes)
{
    return fromBytes(bytes.data(), bytes.size());
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

bool operator==(const Address &lhs, const Address &rhs)
{
    if (lhs.adressFamily() == rhs.adressFamily())
    {
        const auto addressLenght = lhs.addressLength();
        return std::equal(lhs.begin(), lhs.begin() + addressLenght, rhs.begin(), rhs.begin() + addressLenght);
    }
    return false;
}
bool operator!=(const Address &lhs, const Address &rhs)
{
    return !(lhs == rhs);
}

} // namespace ip
} // namespace monkas
