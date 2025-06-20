#include <algorithm>
#include <cstdio>
#include <ostream>

#include <ethernet/Address.hpp>

namespace monkas::ethernet
{

Address::Address()
    : std::array<uint8_t, ADDR_LEN> {}
{
}

auto Address::fromBytes(const uint8_t* bytes, size_type len) -> Address
{
    if (bytes == nullptr || len != ADDR_LEN) {
        return {};
    }
    Address r;
    std::copy_n(bytes, len, r.begin());
    r.m_isValid = true;
    return r;
}

auto Address::fromBytes(const std::array<uint8_t, ADDR_LEN>& bytes) -> Address
{
    return fromBytes(bytes.data(), bytes.size());
}

auto Address::toString() const -> std::string
{
    if (!m_isValid) {
        return {"Invalid"};
    }
    constexpr auto LEN = 17;  // 6*2 hex digits + 5 colons
    std::array<char, LEN> buf {};
    int idx = 0;
    int i = 0;
    constexpr auto LOWER_NIBBLE_SHIFT = 0x4U;
    constexpr auto UPPER_NIBBLE_MASK = 0xFU;
    for (const auto octet : *this) {
        buf[idx++] = "0123456789abcdef"[octet >> LOWER_NIBBLE_SHIFT];
        buf[idx++] = "0123456789abcdef"[octet & UPPER_NIBBLE_MASK];
        if (i < size() - 1) {
            buf[idx++] = ':';
        }
        i++;
    }
    return {buf.data(), buf.size()};
}

Address::operator bool() const
{
    return m_isValid;
}

auto Address::operator<=>(const Address& other) const -> std::strong_ordering
{
    if (!m_isValid && !other.m_isValid) {
        return std::strong_ordering::equal;
    }
    if (!m_isValid) {
        return std::strong_ordering::less;
    }
    if (!other.m_isValid) {
        return std::strong_ordering::greater;
    }
    return std::lexicographical_compare_three_way(begin(), end(), other.begin(), other.end());
}

auto Address::operator==(const Address& other) const -> bool
{
    if (!m_isValid || !other.m_isValid) {
        return false;
    }
    return std::equal(begin(), end(), other.begin());
}

auto operator<<(std::ostream& o, const Address& a) -> std::ostream&
{
    return o << a.toString();
}

}  // namespace monkas::ethernet
