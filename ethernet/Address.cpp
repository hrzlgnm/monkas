#include "Address.h"

#include <algorithm>
#include <cstdio>
#include <sstream>

namespace monkas
{
namespace ethernet
{

Address Address::fromBytes(const uint8_t *bytes, size_type len)
{
    if (len == ADDR_LEN)
    {
        Address r;
        std::copy_n(bytes, len, r.begin());
        return r;
    }
    return Address();
}

std::string Address::toString() const
{
    char buf[18]; // 6*2 chars + 5 sep + nul
    std::snprintf(buf, sizeof(buf), "%02hx%c%02hx%c%02hx%c%02hx%c%02hx%c%02hx", operator[](0), ':', operator[](1),
                  ':', operator[](2), ':', operator[](3), ':', operator[](4), ':', operator[](5));
    return std::string(buf);
}

Address::operator bool() const
{
    return std::any_of(cbegin(), cend(), [](auto c) { return c != 0; });
}

std::ostream &operator<<(std::ostream &o, const Address &a)
{
    return o << a.toString();
}

} // namespace ethernet
} // namespace monkas
