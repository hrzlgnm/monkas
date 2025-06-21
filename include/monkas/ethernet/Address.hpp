#pragma once

#include <array>
#include <cstdint>
#include <iosfwd>

#include <fmt/ostream.h>

namespace monkas::ethernet
{
constexpr auto ADDR_LEN = 6;

class Address : public std::array<uint8_t, ADDR_LEN>
{
  public:
    Address();
    [[nodiscard]] auto toString() const -> std::string;

    static auto fromBytes(const uint8_t* bytes, size_type len) -> Address;
    static auto fromBytes(const std::array<uint8_t, ADDR_LEN>& bytes) -> Address;
};

auto operator<<(std::ostream& o, const Address& a) -> std::ostream&;

}  // namespace monkas::ethernet

template<>
struct fmt::formatter<monkas::ethernet::Address> : fmt::ostream_formatter
{
};
