#include <compare>
#include <ostream>

#include <linux/rtnetlink.h>
#include <network/Address.hpp>

namespace monkas::network
{

Address::Address(const ip::Address& address,
                 const std::optional<ip::Address>& broadcast,
                 const uint8_t prefixLen,
                 const Scope scope,
                 const AddressFlags& flags,
                 const AddressAssignmentProtocol proto)
    : m_ip {address}
    , m_brd {broadcast}
    , m_prefixlen {prefixLen}
    , m_scope {scope}
    , m_flags {flags}
    , m_prot {proto}
{
}

auto Address::family() const -> Family
{
    return m_ip.family();
}

auto Address::isV4() const -> bool
{
    return m_ip.isV4();
}

auto Address::isV6() const -> bool
{
    return m_ip.isV6();
}

auto Address::ip() const -> const ip::Address&
{
    return m_ip;
}

auto Address::broadcast() const -> std::optional<ip::Address>
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

auto Address::flags() const -> AddressFlags
{
    return m_flags;
}

auto Address::addressAssignmentProtocol() const -> AddressAssignmentProtocol
{
    return m_prot;
}

auto Address::operator<=>(const Address& other) const -> std::strong_ordering
{
    if (const auto cmp = m_ip <=> other.m_ip; cmp != std::strong_ordering::equivalent) {
        return cmp;
    }
    if (const auto cmp = m_brd <=> other.m_brd; cmp != std::strong_ordering::equivalent) {
        return cmp;
    }
    if (const auto cmp = m_prefixlen <=> other.m_prefixlen; cmp != std::strong_ordering::equivalent) {
        return cmp;
    }
    if (const auto cmp = m_scope <=> other.m_scope; cmp != std::strong_ordering::equivalent) {
        return cmp;
    }
    if (const auto cmp = m_flags <=> other.m_flags; cmp != std::strong_ordering::equivalent) {
        return cmp;
    }
    return m_prot <=> other.m_prot;
}

auto fromRtnlScope(const uint8_t rtnlScope) -> Scope
{
    switch (rtnlScope) {
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

auto operator<<(std::ostream& o, const Scope a) -> std::ostream&
{
    switch (a) {
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

auto operator<<(std::ostream& o, const AddressFlag a) -> std::ostream&
{
    using enum AddressFlag;
    switch (a) {
        case Temporary:
            return o << "Temporary";
        case NoDuplicateAddressDetection:
            return o << "NoDuplicateAddressDetection";
        case Optimistic:
            return o << "Optimistic";
        case HomeAddress:
            return o << "HomeAddress";
        case DuplicateAddressDetectionFailed:
            return o << "DuplicateAddressDetectionFailed";
        case Deprecated:
            return o << "Deprecated";
        case Tentative:
            return o << "Tentative";
        case Permanent:
            return o << "Permanent";
        case ManagedTemporaryAddress:
            return o << "ManagedTemporaryAddress";
        case NoPrefixRoute:
            return o << "NoPrefixRoute";
        case MulticastAutoJoin:
            return o << "MulticastAutoJoin";
        case StablePrivacy:
            return o << "StablePrivacy";
        default:
            return o << "Unknown Flag: 0x" << std::hex << static_cast<int>(a);
    }
}

auto operator<<(std::ostream& o, const AddressFlags& a) -> std::ostream&
{
    return o << a.toString();
}

auto operator<<(std::ostream& o, AddressAssignmentProtocol a) -> std::ostream&
{
    using enum AddressAssignmentProtocol;
    switch (a) {
        case Unspecified:
            return o << "Unspecified";
        case KernelLoopback:
            return o << "KernelLoopback";
        case KernelRouterAdvertisement:
            return o << "KernelRouterAdvertisement";
        case KernelLinkLocal:
            return o << "KernelLinkLocal";
        default:
            return o << "Unknown Address AddressAssignmentProtocol: 0x" << std::hex << static_cast<int>(a);
    }
}

auto operator<<(std::ostream& o, const Address& a) -> std::ostream&
{
    o << a.family() << " " << a.ip() << "/" << static_cast<int>(a.prefixLength());
    o << " scope " << a.scope();
    if (a.broadcast().has_value()) {
        o << " brd " << a.broadcast().value();
    }
    if (a.flags().any()) {
        o << " <" << a.flags() << ">";
    }
    if (a.addressAssignmentProtocol() != AddressAssignmentProtocol::Unspecified) {
        o << " proto " << a.addressAssignmentProtocol();
    }
    return o;
}

}  // namespace monkas::network
