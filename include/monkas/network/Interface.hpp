#pragma once

#include <iosfwd>
#include <string>
#include <tuple>

#include <fmt/ostream.h>

namespace monkas::network
{
class Interface
{
  public:
    Interface() = default;
    Interface(std::uint32_t index, std::string name);

    [[nodiscard]] constexpr auto index() const -> uint32_t { return m_index; }

    [[nodiscard]] constexpr auto name() const -> const std::string& { return m_name; }

    [[nodiscard]] constexpr auto operator<=>(const Interface& other) const noexcept -> std::strong_ordering
    {
        return std::tie(m_index, m_name) <=> std::tie(other.m_index, other.m_name);
    }

    [[nodiscard]] constexpr auto operator==(const Interface& other) const -> bool = default;

  private:
    uint32_t m_index {};
    std::string m_name;
};

auto operator<<(std::ostream& os, const Interface& iface) -> std::ostream&;

}  // namespace monkas::network

template<>
struct fmt::formatter<monkas::network::Interface> : ostream_formatter
{
};
