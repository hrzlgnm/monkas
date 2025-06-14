#include <network/Interface.hpp>
#include <ostream>

namespace monkas::network
{
Interface::Interface(int index, const std::string &name)
    : m_index(index)
    , m_name(name)
{
}

std::ostream &operator<<(std::ostream &os, const Interface &iface)
{
    return os << iface.index() << ": " << iface.name();
}

} // namespace monkas::network
