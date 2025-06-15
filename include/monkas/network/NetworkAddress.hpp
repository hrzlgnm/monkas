#pragma once

#include <fmt/ostream.h>
#include <ip/Address.hpp>

namespace monkas::network
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
auto operator<<(std::ostream &o, AddressScope a) -> std::ostream &;

auto fromRtnlScope(uint8_t rtnlScope) -> AddressScope;

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

    [[nodiscard]] auto addressFamily() const -> AddressFamily;
    [[nodiscard]] auto ip() const -> const ip::Address &;
    [[nodiscard]] auto broadcast() const -> const ip::Address &;
    [[nodiscard]] auto prefixLength() const -> uint8_t;
    [[nodiscard]] auto scope() const -> AddressScope;
    [[nodiscard]] auto flags() const -> uint32_t;

  private:
    AddressFamily m_af{AddressFamily::Unspecified};
    ip::Address m_ip;
    ip::Address m_brd;
    uint8_t m_prefixlen{};
    AddressScope m_scope{AddressScope::Nowhere};
    uint32_t m_flags{};
};

auto operator==(const NetworkAddress &lhs, const NetworkAddress &rhs) -> bool;
auto operator!=(const NetworkAddress &lhs, const NetworkAddress &rhs) -> bool;
auto operator<(const NetworkAddress &lhs, const NetworkAddress &rhs) -> bool;
auto operator>=(const NetworkAddress &lhs, const NetworkAddress &rhs) -> bool;

auto operator<<(std::ostream &o, const NetworkAddress &a) -> std::ostream &;

} // namespace monkas::network

template <> struct fmt::formatter<monkas::network::NetworkAddress> : fmt::ostream_formatter
{
};
