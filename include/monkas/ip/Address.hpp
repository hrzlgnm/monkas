#pragma once

#include <array>
#include <compare>
#include <cstdint>
#include <expected>
#include <iosfwd>
#include <string>
#include <variant>

#include <fmt/format.h>
#include <fmt/ostream.h>

namespace monkas::ip
{

enum class Family : uint8_t
{
    IPv4,
    IPv6,
};

auto asLinuxAf(Family f) -> int;
auto operator<<(std::ostream& o, Family f) -> std::ostream&;

constexpr auto IPV6_ADDR_LEN = 16;
constexpr auto IPV4_ADDR_LEN = 4;

using IpV4Bytes = std::array<uint8_t, IPV4_ADDR_LEN>;
using IpV6Bytes = std::array<uint8_t, IPV6_ADDR_LEN>;
using Bytes = std::variant<IpV4Bytes, IpV6Bytes>;

class Address
{
  public:
    Address();
    explicit Address(const IpV4Bytes& bytes);
    explicit Address(const IpV6Bytes& bytes);

    [[nodiscard]] auto toString() const -> std::string;

    static auto fromString(const std::string& address) -> std::expected<Address, std::string>;

    [[nodiscard]] auto isV4() const -> bool;

    [[nodiscard]] auto isV6() const -> bool;

    [[nodiscard]] auto isMulticast() const -> bool;
    [[nodiscard]] auto isLinkLocal() const -> bool;
    [[nodiscard]] auto isUniqueLocal() const -> bool;
    [[nodiscard]] auto isLoopback() const -> bool;
    [[nodiscard]] auto isBroadcast() const -> bool;

    [[nodiscard]] auto ip() const -> const Address&;
    [[nodiscard]] auto broadcast() const -> const Address&;

    [[nodiscard]] auto family() const -> Family;

    [[nodiscard]] auto operator<=>(const Address& rhs) const noexcept -> std::strong_ordering;
    [[nodiscard]] auto operator==(const Address& rhs) const noexcept -> bool = default;

  private:
    Bytes m_bytes;
};

auto operator<<(std::ostream& o, const Address& a) -> std::ostream&;

}  // namespace monkas::ip

template<>
struct fmt::formatter<monkas::ip::Address> : fmt::ostream_formatter
{
};
