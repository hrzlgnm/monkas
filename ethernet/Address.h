#ifndef MONKAS_ETHERNET_ADDESS_H
#define MONKAS_ETHERNET_ADDESS_H

#include <array>
#include <cstdint>
#include <functional>
#include <iosfwd>

namespace monkas
{
namespace ethernet
{
constexpr auto ADDR_LEN = 6;
class Address : public std::array<uint8_t, ADDR_LEN>
{
  public:
    Address() = default;
    static Address fromBytes(const uint8_t *bytes, size_type len);
    std::string toString() const;
    explicit operator bool() const;
};
std::ostream &operator<<(std::ostream &o, const Address &a);

} // namespace ethernet
} // namespace monkas

namespace std
{
// specialize std::hash<> for ethernet::Address
template <> struct hash<monkas::ethernet::Address>
{
    size_t operator()(const monkas::ethernet::Address &a) const noexcept;
};
} // namespace std

#endif
