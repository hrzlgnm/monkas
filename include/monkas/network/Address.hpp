#pragma once

#include <compare>
#include <optional>

#include <fmt/ostream.h>
#include <ip/Address.hpp>

namespace monkas::network
{
using Family = ip::Family;

enum class Scope : uint8_t
{
    Global,
    Site,
    Link,
    Host,
    Nowhere,
};
auto operator<<(std::ostream& o, Scope a) -> std::ostream&;

auto fromRtnlScope(uint8_t rtnlScope) -> Scope;

class Address
{
  public:
    Address() = default;
    Address(const ip::Address& address,
            std::optional<ip::Address> broadcast,
            uint8_t prefixLen,
            Scope scope,
            uint32_t flags,
            uint8_t proto);

    [[nodiscard]] auto family() const -> Family;

    [[nodiscard]] auto isV4() const -> bool;
    [[nodiscard]] auto isV6() const -> bool;

    [[nodiscard]] auto ip() const -> const ip::Address&;
    [[nodiscard]] auto broadcast() const -> std::optional<ip::Address>;
    [[nodiscard]] auto prefixLength() const -> uint8_t;
    [[nodiscard]] auto scope() const -> Scope;
    [[nodiscard]] auto flags() const -> uint32_t;
    [[nodiscard]] auto proto() const -> uint8_t;

    [[nodiscard]] auto operator<=>(const Address& other) const -> std::strong_ordering;
    [[nodiscard]] auto operator==(const Address& other) const -> bool = default;

  private:
    ip::Address m_ip;
    std::optional<ip::Address> m_brd;
    uint8_t m_prefixlen {};
    Scope m_scope {Scope::Nowhere};
    uint32_t m_flags {};
    uint8_t m_prot {0};
};

auto operator<<(std::ostream& o, const Address& a) -> std::ostream&;

}  // namespace monkas::network

template<>
struct fmt::formatter<monkas::network::Address> : fmt::ostream_formatter
{
};
