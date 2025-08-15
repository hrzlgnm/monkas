#include <array>
#include <ostream>
#include <stdexcept>

#include <net/if.h>
#include <network/Interface.hpp>

namespace monkas::network
{

auto Interface::from(std::string name) -> Interface
{
    const auto index = if_nametoindex(name.c_str());
    if (index == 0) {
        throw std::invalid_argument("Failed to convert interface name to index: " + name);
    }
    return Interface {index, std::move(name)};
}

auto Interface::from(std::uint32_t index) -> Interface
{
    std::array<char, IF_NAMESIZE> nameBuffer {};
    auto* const intfName = if_indextoname(index, nameBuffer.data());
    if (intfName == nullptr) {
        throw std::invalid_argument("Failed to convert interface index " + std::to_string(index) + " to name");
    }
    return Interface {index, intfName};
}

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
