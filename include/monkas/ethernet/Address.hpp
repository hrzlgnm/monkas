#pragma once

#include <array>
#include <cstdint>
#include <fmt/ostream.h>
#include <iosfwd>

namespace monkas::ethernet
{
constexpr auto addrLen = 6;

class Address : public std::array<uint8_t, addrLen>
{
  public:
    Address() = default;
    [[nodiscard]] auto toString() const -> std::string;
    /**
     * @return true if any of the octets of the address is not zero
     */
    explicit operator bool() const;

    static auto fromBytes(const uint8_t *bytes, size_type len) -> Address;
    static auto fromBytes(const std::array<uint8_t, addrLen> &bytes) -> Address;
};

auto operator<<(std::ostream &o, const Address &a) -> std::ostream &;

} // namespace monkas::ethernet

template <> struct fmt::formatter<monkas::ethernet::Address> : fmt::ostream_formatter
{
};
