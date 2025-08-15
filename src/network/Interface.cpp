#include <array>
#include <cerrno>
#include <ostream>
#include <system_error>

#include <net/if.h>
#include <network/Interface.hpp>

namespace monkas::network
{

auto Interface::fromName(std::string name) -> Interface
{
    const auto index = if_nametoindex(name.c_str());
    if (index == 0) {
        const auto err = errno;
        throw std::system_error(err, std::system_category(), "if_nametoindex(\"" + name + "\") failed");
    }
    return Interface {index, std::move(name)};
}

auto Interface::fromIndex(std::uint32_t index) -> Interface
{
    std::array<char, IF_NAMESIZE> nameBuffer {};
    auto* const intfName = if_indextoname(index, nameBuffer.data());
    if (intfName == nullptr) {
        const auto err = errno;
        throw std::system_error(err, std::system_category(), "if_indextoname(" + std::to_string(index) + ") failed");
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
