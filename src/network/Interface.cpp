#include <ostream>

#include <network/Interface.hpp>

namespace monkas::network
{

Interface::Interface(const uint32_t index, std::string name)
    : m_index {index}
    , m_name {std::move(name)}
{
}

auto operator<<(std::ostream& os, const Interface& iface) -> std::ostream&
{
    return os << iface.index() << ": " << iface.name() << ":";
}

}  // namespace monkas::network
