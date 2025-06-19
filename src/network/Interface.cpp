#include <network/Interface.hpp>
#include <ostream>

namespace monkas::network
{

Interface::Interface(uint32_t index, std::string name)
    : m_index{index}
    , m_name{std::move(name)}
{
}

auto operator<<(std::ostream &os, const Interface &iface) -> std::ostream &
{
    return os << iface.index() << ": " << iface.name();
}

} // namespace monkas::network
