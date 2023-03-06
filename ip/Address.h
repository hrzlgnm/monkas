#ifndef MONKAS_IP_ADDRESS_H
#define MONKAS_IP_ADDRESS_H

#include <array>
#include <cstdint>
#include <iosfwd>
#include <spdlog/fmt/fmt.h>
#include <string>

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

    AddressFamily adressFamily() const;
    size_type addressLength() const;
    std::string toString() const;

    static Address fromBytes(const uint8_t *bytes, size_type len);

  private:
    AddressFamily m_addressFamily{AddressFamily::Unspecified};
};
std::ostream &operator<<(std::ostream &o, const Address &a);
bool operator<(const Address &lhs, const Address &rhs);

} // namespace ip
} // namespace monkas
template <> struct fmt::formatter<monkas::ip::Address> : fmt::formatter<std::string>
{
    auto format(const monkas::ip::Address &addr, format_context &ctx) -> decltype(ctx.out())
    {
        return format_to(ctx.out(), "{}", addr.toString());
    }
};
#endif
