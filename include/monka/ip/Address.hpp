#pragma once

#include <array>
#include <cstdint>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <iosfwd>
#include <string>
#include <string_view>

namespace monkas
{
namespace ip
{

enum class AddressFamily
{
    Unspecified,
    IPv4,
    IPv6,
};

int asLinuxAf(AddressFamily f);
std::ostream &operator<<(std::ostream &o, AddressFamily a);

constexpr auto IPV6_ADDR_LEN = 16;
constexpr auto IPV4_ADDR_LEN = 4;
class Address : public std::array<uint8_t, IPV6_ADDR_LEN>
{
  public:
    Address() = default;

    /**
     * @returns true if address is not unspecified
     */
    explicit operator bool() const;

    AddressFamily addressFamily() const;
    size_type addressLength() const;
    std::string toString() const;

    static Address fromString(std::string_view address);
    static Address fromBytes(const uint8_t *bytes, size_type len);
    static Address fromBytes(const std::array<uint8_t, IPV4_ADDR_LEN> &bytes);
    static Address fromBytes(const std::array<uint8_t, IPV6_ADDR_LEN> &bytes);

  private:
    AddressFamily m_addressFamily{AddressFamily::Unspecified};
};
std::ostream &operator<<(std::ostream &o, const Address &a);
bool operator<(const Address &lhs, const Address &rhs);
bool operator==(const Address &lhs, const Address &rhs);
bool operator!=(const Address &lhs, const Address &rhs);

} // namespace ip
} // namespace monkas

template <> struct fmt::formatter<monkas::ip::Address> : fmt::ostream_formatter
{
};
