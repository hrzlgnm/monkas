#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
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

constexpr std::size_t IPV6_ADDR_LEN = 16;
constexpr std::size_t IPV4_ADDR_LEN = 4;

using V4Bytes = std::array<uint8_t, IPV4_ADDR_LEN>;
using V6Bytes = std::array<uint8_t, IPV6_ADDR_LEN>;
using Bytes = std::variant<V4Bytes, V6Bytes>;

class Address
{
  public:
    Address();
    explicit Address(const V4Bytes& bytes);
    explicit Address(const V6Bytes& bytes);

    [[nodiscard]] auto toString() const -> std::string;

    static auto fromString(const std::string& address) noexcept(false) -> Address;

    [[nodiscard]] auto isV4() const -> bool;
    [[nodiscard]] auto isV6() const -> bool;

    [[nodiscard]] auto isMulticast() const -> bool;
    [[nodiscard]] auto isUnicastLinkLocal() const -> bool;
    [[nodiscard]] auto isUniqueLocal() const -> bool;
    [[nodiscard]] auto isLoopback() const -> bool;
    [[nodiscard]] auto isBroadcast() const -> bool;

    [[nodiscard]] auto ip() const -> const Address&;
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
