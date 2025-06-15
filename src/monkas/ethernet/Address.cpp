#include <ethernet/Address.hpp>

#include <algorithm>
#include <cstdio>
#include <ostream>

namespace monkas::ethernet
{

auto Address::fromBytes(const uint8_t *bytes, size_type len) -> Address
{
    if (len == ADDR_LEN)
    {
        Address r;
        std::copy_n(bytes, len, r.begin());
        return r;
    }
    return {};
}

auto Address::fromBytes(const std::array<uint8_t, ADDR_LEN> &bytes) -> Address
{
    return fromBytes(bytes.data(), bytes.size());
}

auto Address::toString() const -> std::string
{
    constexpr auto len = 17; // 6*2 hex digits + 5 colons
    std::array<char, len> buf{};
    int idx = 0;
    int i = 0;
    constexpr auto lowerNibbleShift = 0x4U;
    constexpr auto upperNibbleMask = 0xFU;
    for (const auto byte : *this)
    {
        buf[idx++] = "0123456789abcdef"[byte >> lowerNibbleShift];
        buf[idx++] = "0123456789abcdef"[byte & upperNibbleMask];
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
