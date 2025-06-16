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

constexpr auto ipV6AddrLen = 16;
constexpr auto ipV4AddrLen = 4;

class Address : public std::array<uint8_t, ipV6AddrLen>
{
  public:
    Address();

    /**
     * @returns true if address is not unspecified
     */
    explicit operator bool() const;

    [[nodiscard]] auto addressFamily() const -> AddressFamily;
    [[nodiscard]] auto addressLength() const -> size_type;
    [[nodiscard]] auto isMappedV4() const -> bool;
    [[nodiscard]] auto toString() const -> std::string;

    static auto fromString(const std::string &address) -> Address;
    static auto fromBytes(const uint8_t *bytes, size_type len) -> Address;
    static auto fromBytes(const std::array<uint8_t, ipV4AddrLen> &bytes) -> Address;
    static auto fromBytes(const std::array<uint8_t, ipV6AddrLen> &bytes) -> Address;

  private:
    AddressFamily m_addressFamily{AddressFamily::Unspecified};
};

auto operator<<(std::ostream &o, const Address &a) -> std::ostream &;
auto operator<(const Address &lhs, const Address &rhs) -> bool;
auto operator<=(const Address &lhs, const Address &rhs) -> bool;
auto operator>(const Address &lhs, const Address &rhs) -> bool;
auto operator>=(const Address &lhs, const Address &rhs) -> bool;
auto operator==(const Address &lhs, const Address &rhs) -> bool;
auto operator!=(const Address &lhs, const Address &rhs) -> bool;

} // namespace monkas::ip

template <> struct fmt::formatter<monkas::ip::Address> : fmt::ostream_formatter
{
};
