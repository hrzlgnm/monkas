#ifndef MONKAS_NETWORK_NETWORKADDRESS_H
#define MONKAS_NETWORK_NETWORKADDRESS_H

#include "ip/Address.h"
#include <sstream>

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
    Nowhere,
};
std::ostream &operator<<(std::ostream &o, AddressScope a);

AddressScope fromRtnlScope(uint8_t rtnlScope);

class NetworkAddress
{
  public:
    NetworkAddress(AddressFamily addresFamily, const ip::Address &address, const ip::Address &broadcast,
                   uint8_t prefixLen, AddressScope scope, uint32_t flags);
    /**
     * @returns true if AddressFamily is not Unspecified
     */
    explicit operator bool() const;

    AddressFamily adressFamily() const;
    const ip::Address &ip() const;
    const ip::Address &broadcast() const;
    uint8_t prefixLength() const;
    AddressScope scope() const;
    uint32_t flags() const;

  private:
    AddressFamily m_af;
    ip::Address m_ip;
    ip::Address m_brd;
    uint8_t m_prefixlen;
    AddressScope m_scope;
    uint32_t m_flags;
};
std::ostream &operator<<(std::ostream &o, const NetworkAddress &a);
bool operator<(const NetworkAddress &lhs, const NetworkAddress &rhs);

} // namespace network
} // namespace monkas
template <> struct fmt::formatter<monkas::network::NetworkAddress> : fmt::formatter<std::string>
{
    auto format(const monkas::network::NetworkAddress &addr, format_context &ctx) -> decltype(ctx.out())
    {
        std::ostringstream strm;
        strm << addr;
        return format_to(ctx.out(), "{}", strm.str());
    }
};

#endif // MONKAS_NETWORK_NETWORKADDRESS_H
