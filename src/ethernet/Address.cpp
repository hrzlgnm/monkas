// Copyright 2023-2025 hrzlgnm
// SPDX-License-Identifier: MIT-0

#include <algorithm>
#include <ostream>

#include <ethernet/Address.hpp>

namespace monkas::ethernet
{

Address::Address()
    : m_bytes {}
{
}

Address::Address(const Bytes& bytes)
    : m_bytes {bytes}
{
}

auto Address::allZeroes() const -> bool
{
    return std::ranges::all_of(m_bytes, [](const uint8_t byte) { return byte == 0; });
}

auto Address::isBroadcast() const -> bool
{
    constexpr uint8_t BROADCAST_BYTE = 0xFF;
    return std::ranges::all_of(m_bytes, [](const uint8_t byte) { return byte == BROADCAST_BYTE; });
}

auto Address::toString() const -> std::string
{
    constexpr auto LEN = 17;  // 6*2 hex digits + 5 colons
    std::array<char, LEN> buf {};
    size_t idx = 0;
    size_t i = 0;
    for (const auto octet : m_bytes) {
        constexpr auto UPPER_NIBBLE_MASK = 0xFU;
        constexpr auto LOWER_NIBBLE_SHIFT = 0x4U;
        buf[idx++] = "0123456789abcdef"[octet >> LOWER_NIBBLE_SHIFT];
        buf[idx++] = "0123456789abcdef"[octet & UPPER_NIBBLE_MASK];
        if (i < ADDR_LEN - 1) {
            buf[idx++] = ':';
        }
        i++;
    }
    return {buf.data(), buf.size()};
}

auto Address::operator<=>(const Address& other) const -> std::strong_ordering
{
    return m_bytes <=> other.m_bytes;
}

auto operator<<(std::ostream& o, const Address& a) -> std::ostream&
{
    return o << a.toString();
}

}  // namespace monkas::ethernet
