#pragma once

#include <array>
#include <compare>
#include <cstdint>
#include <fmt/ostream.h>
#include <iosfwd>

namespace monkas::ethernet
{
constexpr auto ADDR_LEN = 6;

class Address : public std::array<uint8_t, ADDR_LEN>
{
  public:
    Address();
    [[nodiscard]] auto toString() const -> std::string;

    /**
     * @return true if actually constructed from bytes
     */
    explicit operator bool() const;

    static auto fromBytes(const uint8_t *bytes, size_type len) -> Address;
    static auto fromBytes(const std::array<uint8_t, ADDR_LEN> &bytes) -> Address;

    [[nodiscard]] auto operator<=>(const Address &) const -> std::strong_ordering;
    [[nodiscard]] auto operator==(const Address &) const -> bool;

  private:
    bool m_isValid{false};
};

auto operator<<(std::ostream &o, const Address &a) -> std::ostream &;

} // namespace monkas::ethernet

template <> struct fmt::formatter<monkas::ethernet::Address> : fmt::ostream_formatter
{
};
