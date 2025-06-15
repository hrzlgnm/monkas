#pragma once

#include <cstdint>
#include <fmt/ostream.h>
#include <iosfwd>
#include <string>
#include <tuple>

namespace monkas::network
{
class Interface
{
  public:
    Interface() = default;
    Interface(std::uint32_t index, std::string name);

    [[nodiscard]] auto index() const -> uint32_t
    {
        return m_index;
    }

    [[nodiscard]] auto name() const -> const std::string &
    {
        return m_name;
    }

    constexpr auto operator<(const Interface &other) const noexcept -> bool
    {
        return std::tie(m_index, m_name) < std::tie(other.m_index, other.m_name);
    }

    constexpr auto operator==(const Interface &other) const -> bool
    {
        return m_index == other.m_index && m_name == other.m_name;
    }

    constexpr auto operator!=(const Interface &other) const -> bool
    {
        return !(*this == other);
    }

  private:
    uint32_t m_index{};
    std::string m_name;
};

auto operator<<(std::ostream &os, const Interface &iface) -> std::ostream &;

} // namespace monkas::network

template <> struct fmt::formatter<monkas::network::Interface> : fmt::ostream_formatter
{
};
