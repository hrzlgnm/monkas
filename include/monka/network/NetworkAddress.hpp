#pragma once

#include <fmt/ostream.h>
#include <ip/Address.hpp>

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
    NetworkAddress() = default;
    NetworkAddress(AddressFamily addressFamily, const ip::Address &address, const ip::Address &broadcast,
                   uint8_t prefixLen, AddressScope scope, uint32_t flags);
    /**
     * @returns true if AddressFamily is not Unspecified
     */
    explicit operator bool() const;

    AddressFamily addressFamily() const;
    const ip::Address &ip() const;
    const ip::Address &broadcast() const;
    uint8_t prefixLength() const;
    AddressScope scope() const;
    uint32_t flags() const;

  private:
    AddressFamily m_af{AddressFamily::Unspecified};
    ip::Address m_ip;
    ip::Address m_brd;
    uint8_t m_prefixlen{};
    AddressScope m_scope{AddressScope::Nowhere};
    uint32_t m_flags{};
};

bool operator==(const NetworkAddress &lhs, const NetworkAddress &rhs);
bool operator!=(const NetworkAddress &lhs, const NetworkAddress &rhs);
bool operator<(const NetworkAddress &lhs, const NetworkAddress &rhs);
bool operator>=(const NetworkAddress &lhs, const NetworkAddress &rhs);

std::ostream &operator<<(std::ostream &o, const NetworkAddress &a);

} // namespace network
} // namespace monkas

template <> struct fmt::formatter<monkas::network::NetworkAddress> : fmt::ostream_formatter
{
};

;
