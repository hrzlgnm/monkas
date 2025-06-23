#pragma once

#include <array>
#include <compare>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>

#include <fmt/format.h>
#include <fmt/ostream.h>

namespace monkas::ip
{

enum class Family : uint8_t
{
    Unspecified,
    IPv4,
    IPv6,
};

auto asLinuxAf(Family f) -> int;
auto operator<<(std::ostream& o, Family a) -> std::ostream&;

constexpr auto IPV6_ADDR_LEN = 16;
constexpr auto IPV4_ADDR_LEN = 4;

using IpV4Bytes = std::array<uint8_t, IPV4_ADDR_LEN>;
using IpV6Bytes = std::array<uint8_t, IPV6_ADDR_LEN>;

class Address : public std::array<uint8_t, IPV6_ADDR_LEN>
{
  public:
    Address();
    [[nodiscard]] auto toString() const -> std::string;

    /**
     * Creates an Address from a string representation.
     * If the string is not a valid address, it returns an unspecified Address.
     */
    static auto fromString(const std::string& address) -> Address;
    /**
     * @returns true if address is not unspecified
     */
    explicit operator bool() const;

    [[nodiscard]] auto isV4() const -> bool { return m_family == Family::IPv4; }

    [[nodiscard]] auto isV6() const -> bool { return m_family == Family::IPv6; }

    [[nodiscard]] auto isUnspecified() const -> bool { return m_family == Family::Unspecified; }

    [[nodiscard]] auto isMulticast() const -> bool;
    [[nodiscard]] auto isLinkLocal() const -> bool;
    [[nodiscard]] auto isUniqueLocal() const -> bool;
    [[nodiscard]] auto isLoopback() const -> bool;
    [[nodiscard]] auto isBroadcast() const -> bool;
    [[nodiscard]] auto isPrivate() const -> bool;
    [[nodiscard]] auto isDocumentation() const -> bool;

    [[nodiscard]] auto isMappedV4() const -> bool;
    [[nodiscard]] auto toMappedV4() const -> std::optional<Address>;

    [[nodiscard]] auto ip() const -> const Address&;
    [[nodiscard]] auto broadcast() const -> const Address&;
    [[nodiscard]] auto prefixLength() const -> uint8_t;

    [[nodiscard]] auto family() const -> Family;
    [[nodiscard]] auto addressLength() const -> size_type;

    static auto fromBytes(const IpV4Bytes& bytes) -> Address;
    static auto fromBytes(const IpV6Bytes& bytes) -> Address;

    // overload the one from std::array<uint8_t, IPV6_ADDR_LEN>
    [[nodiscard]] auto operator<=>(const Address& rhs) const noexcept -> std::strong_ordering;
    // overload the one from std::array<uint8_t, IPV6_ADDR_LEN>
    [[nodiscard]] auto operator==(const Address& rhs) const noexcept -> bool;

  private:
    static auto fromBytes(const uint8_t* bytes, size_type len) -> Address;
    [[nodiscard]] auto ipv4() const -> uint32_t;

    Family m_family {Family::Unspecified};
};

auto operator<<(std::ostream& o, const Address& a) -> std::ostream&;

}  // namespace monkas::ip

template<>
struct fmt::formatter<monkas::ip::Address> : fmt::ostream_formatter
{
};
