#pragma once

#include <array>
#include <cstdint>
#include <iosfwd>
#include <spdlog/fmt/fmt.h>

namespace monkas
{
namespace ethernet
{
constexpr auto ADDR_LEN = 6;
class Address : public std::array<uint8_t, ADDR_LEN>
{
  public:
    Address() = default;
    std::string toString() const;
    /**
     * @return true if any of the octets of the address is not zero
     */
    explicit operator bool() const;

    static Address fromBytes(const uint8_t *bytes, size_type len);
    static Address fromBytes(const std::array<uint8_t, ADDR_LEN> &bytes);
};
std::ostream &operator<<(std::ostream &o, const Address &a);

} // namespace ethernet
} // namespace monkas
template <> struct fmt::formatter<monkas::ethernet::Address> : fmt::formatter<std::string>
{
    auto format(const monkas::ethernet::Address &addr, format_context &ctx) -> decltype(ctx.out())
    {
        return format_to(ctx.out(), "{}", addr.toString());
    }
};
