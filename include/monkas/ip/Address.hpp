#pragma once

#include <array>
#include <cstdint>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <iosfwd>
#include <string>

namespace monkas::ip
{

enum class AddressFamily : uint8_t
{
    Unspecified,
    IPv4,
    IPv6,
};

auto asLinuxAf(AddressFamily f) -> int;
auto operator<<(std::ostream &o, AddressFamily a) -> std::ostream &;

constexpr auto IPV6_ADDR_LEN = 16;
constexpr auto IPV4_ADDR_LEN = 4;

class Address : public std::array<uint8_t, IPV6_ADDR_LEN>
{
  public:
    Address();
    [[nodiscard]] auto toString() const -> std::string;

    /**
     * Creates an Address from a string representation.
     * If the string is not a valid address, it returns an unspecified Address.
     */
    static auto fromString(const std::string &address) -> Address;
    /**
     * @returns true if address is not unspecified
     */
    explicit operator bool() const;

    [[nodiscard]] auto addressFamily() const -> AddressFamily;
    [[nodiscard]] auto addressLength() const -> size_type;
    [[nodiscard]] auto isMappedV4() const -> bool;

    static auto fromBytes(const uint8_t *bytes, size_type len) -> Address;
    static auto fromBytes(const std::array<uint8_t, IPV4_ADDR_LEN> &bytes) -> Address;
    static auto fromBytes(const std::array<uint8_t, IPV6_ADDR_LEN> &bytes) -> Address;

  private:
    AddressFamily m_addressFamily{AddressFamily::Unspecified};
};

auto operator<<(std::ostream &o, const Address &a) -> std::ostream &;

auto operator<(const Address &lhs, const Address &rhs) noexcept -> bool;
auto operator<=(const Address &lhs, const Address &rhs) noexcept -> bool;
auto operator>(const Address &lhs, const Address &rhs) noexcept -> bool;
auto operator>=(const Address &lhs, const Address &rhs) noexcept -> bool;
auto operator==(const Address &lhs, const Address &rhs) noexcept -> bool;
auto operator!=(const Address &lhs, const Address &rhs) noexcept -> bool;

} // namespace monkas::ip

template <> struct fmt::formatter<monkas::ip::Address> : fmt::ostream_formatter
{
};
