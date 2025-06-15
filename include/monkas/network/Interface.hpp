#pragma once

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
    Interface(int index, const std::string &name);

    inline int index() const
    {
        return m_index;
    }

    inline const std::string &name() const
    {
        return m_name;
    }

    constexpr bool operator<(const Interface &other) const noexcept
    {
        return std::tie(m_index, m_name) < std::tie(other.m_index, other.m_name);
    }

    constexpr bool operator==(const Interface &other) const
    {
        return m_index == other.m_index && m_name == other.m_name;
    }

    constexpr bool operator!=(const Interface &other) const
    {
        return !(*this == other);
    }

  private:
    int m_index{};
    std::string m_name;
};

std::ostream &operator<<(std::ostream &os, const Interface &iface);

} // namespace monkas::network

template <> struct fmt::formatter<monkas::network::Interface> : fmt::ostream_formatter
{
};
