#include <ethernet/Address.hpp>

#include <algorithm>
#include <cstdio>
#include <ostream>

namespace monkas::ethernet
{

auto Address::fromBytes(const uint8_t *bytes, size_type len) -> Address
{
    if (bytes == nullptr || len != ADDR_LEN)
    {
        return {};
    }
    Address r;
    std::copy_n(bytes, len, r.begin());
    return r;
}

auto Address::fromBytes(const std::array<uint8_t, ADDR_LEN> &bytes) -> Address
{
    return fromBytes(bytes.data(), bytes.size());
}

auto Address::toString() const -> std::string
{
    constexpr auto LEN = 17; // 6*2 hex digits + 5 colons
    std::array<char, LEN> buf{};
    int idx = 0;
    int i = 0;
    constexpr auto LOWER_NIBBLE_SHIFT = 0x4U;
    constexpr auto UPPER_NIBBLE_MASK = 0xFU;
    for (const auto octet : *this)
    {
        buf[idx++] = "0123456789abcdef"[octet >> LOWER_NIBBLE_SHIFT];
        buf[idx++] = "0123456789abcdef"[octet & UPPER_NIBBLE_MASK];
        if (i < size() - 1)
        {
            buf[idx++] = ':';
        }
        i++;
    }
    return {buf.data(), buf.size()};
}

Address::operator bool() const
{
    return std::any_of(cbegin(), cend(), [](auto c) { return c != 0; });
}

auto operator<<(std::ostream &o, const Address &a) -> std::ostream &
{
    return o << a.toString();
}

} // namespace monkas::ethernet
