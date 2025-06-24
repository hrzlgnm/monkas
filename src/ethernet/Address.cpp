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

auto Address::toString() const -> std::string
{
    constexpr auto LEN = 17;  // 6*2 hex digits + 5 colons
    std::array<char, LEN> buf {};
    int idx = 0;
    int i = 0;
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
