#ifndef MONKAS_IP_ADDRESS_H
#define MONKAS_IP_ADDRESS_H

#include <array>
#include <cstdint>
#include <string>
namespace monkas
{
namespace ip
{
enum class AddressFamily
{
    Unspecified,
    IPv4,
    IPv6,
};
int asLinuxAf(AddressFamily f);
std::ostream &operator<<(std::ostream &o, AddressFamily a);

constexpr auto IPV6_ADDR_LEN = 16;
constexpr auto IPV4_ADDR_LEN = 4;
class Address : public std::array<uint8_t, IPV6_ADDR_LEN>
{
  public:
    Address() = default;
    explicit operator bool() const;
    AddressFamily adressFamily() const;
    std::string toString() const;

    static Address fromBytes(const uint8_t *bytes, size_type len);

  private:
    AddressFamily m_addressFamily{AddressFamily::Unspecified};
};
std::ostream &operator<<(std::ostream &o, const Address &a);

} // namespace ip
} // namespace monkas
namespace std
{
// specialize std::hash<> for ethernet::Address
template <> struct hash<monkas::ip::Address>
{
    size_t operator()(const monkas::ip::Address &a) const noexcept;
};
} // namespace std

#endif
