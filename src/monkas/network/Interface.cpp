#include <iostream>
#include <network/Interface.hpp>

namespace monkas::network
{
Interface::Interface(uint32_t index, const std::string &name)
    : m_index(index)
    , m_name(name)
{
}

std::ostream &operator<<(std::ostream &os, const Interface &iface)
{
    return os << iface.index() << ": " << iface.name();
}

} // namespace monkas::network
