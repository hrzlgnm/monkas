#ifndef MONKAS_NETWORK_NETWORKADDRESS_H
#define MONKAS_NETWORK_NETWORKADDRESS_H

#include "ip/Address.h"

namespace monkas
{
namespace network
{
using AddressFamily = ip::AddressFamily;

enum class AddressScope : uint8_t
{
    Global,
    Site,
    Link,
    Host,
    NoWhere,
};
std::ostream &operator<<(std::ostream &o, AddressScope a);

AddressScope fromRtnlScope(uint8_t rtnlScope);

class NetworkAddress
{
  public:
    NetworkAddress(AddressFamily addresFamily, const ip::Address &address, const ip::Address &broadcast,
                   uint8_t prefixLen, AddressScope scope, uint32_t flags);
    explicit operator bool() const;
    AddressFamily adressFamily() const;
    const ip::Address &ip() const;
    const ip::Address &broadcast() const;
    uint8_t prefixLenght() const;
    AddressScope scope() const;
    uint32_t flags() const;

  private:
    friend bool operator<(const NetworkAddress &a, const NetworkAddress &b)
    {
        return a.m_ip < b.m_ip;
    }
    AddressFamily m_addressFamily;
    ip::Address m_ip;
    ip::Address m_broadcast;
    uint8_t m_prefixLenght;
    AddressScope m_scope;
    uint32_t m_flags;
};
std::ostream &operator<<(std::ostream &o, const NetworkAddress &a);

} // namespace network
} // namespace monkas

#endif // MONKAS_NETWORK_NETWORKADDRESS_H
