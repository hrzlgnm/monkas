// Copyright 2023-2025 hrzlgnm
// SPDX-License-Identifier: MIT-0

#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <iosfwd>

#include <fmt/ostream.h>

namespace monkas::ethernet
{

constexpr std::size_t ADDR_LEN = 6;
using Bytes = std::array<uint8_t, ADDR_LEN>;

class Address
{
  public:
    Address();
    explicit Address(const Bytes& bytes);

    [[nodiscard]] auto allZeroes() const -> bool;
    [[nodiscard]] auto isBroadcast() const -> bool;
    [[nodiscard]] auto toString() const -> std::string;

    auto operator<=>(const Address& other) const -> std::strong_ordering;
    auto operator==(const Address& other) const -> bool = default;

  private:
    Bytes m_bytes;
};

auto operator<<(std::ostream& o, const Address& a) -> std::ostream&;

}  // namespace monkas::ethernet

template<>
struct fmt::formatter<monkas::ethernet::Address> : ostream_formatter
{
};
