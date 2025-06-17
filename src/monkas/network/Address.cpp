#include <linux/rtnetlink.h>
#include <network/Address.hpp>
#include <ostream>

namespace monkas::network
{

Address::Address(const ip::Address &address, const ip::Address &broadcast, uint8_t prefixLen, Scope scope,
                 uint32_t flags)
    : m_ip{address}
    , m_brd{broadcast}
    , m_prefixlen{prefixLen}
    , m_scope{scope}
    , m_flags{flags}
{
}

Address::operator bool() const
{
    return m_ip.operator bool();
}

auto Address::family() const -> Family
{
    return m_ip.family();
}

auto Address::ip() const -> const ip::Address &
{
    return m_ip;
}

auto Address::broadcast() const -> const ip::Address &
{
    return m_brd;
}

auto Address::prefixLength() const -> uint8_t
{
    return m_prefixlen;
}

auto Address::scope() const -> Scope
{
    return m_scope;
}

auto Address::flags() const -> uint32_t
{
    return m_flags;
}

auto Address::operator<=>(const Address &other) const -> std::strong_ordering
{
    if (auto cmp = m_ip <=> other.m_ip; cmp != 0)
    {
        return cmp;
    }
    if (auto cmp = m_brd <=> other.m_brd; cmp != 0)
    {
        return cmp;
    }
    if (auto cmp = m_prefixlen <=> other.m_prefixlen; cmp != 0)
    {
        return cmp;
    }
    if (auto cmp = m_scope <=> other.m_scope; cmp != 0)
    {
        return cmp;
    }
    return m_flags <=> other.m_flags;
}

auto fromRtnlScope(uint8_t rtnlScope) -> Scope
{
    switch (rtnlScope)
    {
    case RT_SCOPE_SITE:
        return Scope::Site;
    case RT_SCOPE_LINK:
        return Scope::Link;
    case RT_SCOPE_HOST:
        return Scope::Host;
    case RT_SCOPE_NOWHERE:
        return Scope::Nowhere;
    case RT_SCOPE_UNIVERSE:
    default:
        return Scope::Global;
    }
}

auto operator<<(std::ostream &o, Scope a) -> std::ostream &
{
    switch (a)
    {
    case Scope::Site:
        o << "site";
        break;
    case Scope::Link:
        o << "link";
        break;
    case Scope::Host:
        o << "host";
        break;
    case Scope::Nowhere:
        o << "nowhere";
        break;
    case Scope::Global:
    default:
        o << "global";
        break;
    }
    return o;
}

auto operator<<(std::ostream &o, const Address &a) -> std::ostream &
{
    o << a.family() << " " << a.ip() << "/" << static_cast<int>(a.prefixLength());
    o << " scope " << a.scope();
    if (a.broadcast())
    {
        o << " brd " << a.broadcast();
    }
    // TODO: to readable
    if (a.flags() != 0U)
    {
        o << " f " << std::hex << a.flags() << std::dec;
    }
    return o;
}

} // namespace monkas::network
