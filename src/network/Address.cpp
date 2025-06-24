#include <ostream>

#include <linux/if_addr.h>
#include <linux/rtnetlink.h>
#include <network/Address.hpp>

namespace monkas::network
{

/**
 * @brief Constructs an Address object with the specified IP address, optional broadcast address, prefix length, scope, flags, and protocol.
 *
 * @param address The primary IP address.
 * @param broadcast Optional broadcast address associated with the IP address.
 * @param prefixLen The prefix length of the network.
 * @param scope The scope of the address (e.g., global, link, host).
 * @param flags Address-specific flags.
 * @param proto Protocol identifier for the address.
 */
Address::Address(const ip::Address& address,
                 std::optional<ip::Address> broadcast,
                 uint8_t prefixLen,
                 Scope scope,
                 uint32_t flags,
                 uint8_t proto)
    : m_ip {address}
    , m_brd {broadcast}
    , m_prefixlen {prefixLen}
    , m_scope {scope}
    , m_flags {flags}
    , m_prot {proto}
{
}

/**
 * @brief Returns the address family of the underlying IP address.
 *
 * @return Family The address family (e.g., IPv4 or IPv6).
 */
auto Address::family() const -> Family
{
    return m_ip.family();
}

auto Address::isV4() const -> bool
{
    return m_ip.isV4();
}

/**
 * @brief Checks if the address is an IPv6 address.
 *
 * @return true if the stored IP address is IPv6, false otherwise.
 */
auto Address::isV6() const -> bool
{
    return m_ip.isV6();
}

/**
 * @brief Returns the underlying IP address associated with this network address.
 *
 * @return Reference to the stored IP address.
 */
auto Address::ip() const -> const ip::Address&
{
    return m_ip;
}

/**
 * @brief Returns the broadcast address associated with this network address, if present.
 *
 * @return std::optional<ip::Address> The broadcast address, or std::nullopt if not set.
 */
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

auto Address::flags() const -> uint32_t
{
    return m_flags;
}

auto Address::proto() const -> uint8_t
{
    return m_prot;
}

auto Address::operator<=>(const Address& other) const -> std::strong_ordering
{
    if (auto cmp = m_ip <=> other.m_ip; cmp != 0) {
        return cmp;
    }
    if (auto cmp = m_brd <=> other.m_brd; cmp != 0) {
        return cmp;
    }
    if (auto cmp = m_prefixlen <=> other.m_prefixlen; cmp != 0) {
        return cmp;
    }
    if (auto cmp = m_scope <=> other.m_scope; cmp != 0) {
        return cmp;
    }
    if (auto cmp = m_flags <=> other.m_flags; cmp != 0) {
        return cmp;
    }
    return m_prot <=> other.m_prot;
}

auto fromRtnlScope(uint8_t rtnlScope) -> Scope
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

auto operator<<(std::ostream& o, Scope a) -> std::ostream&
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

/**
 * @brief Outputs a human-readable representation of an Address to a stream.
 *
 * The output includes the address family, IP address, prefix length, scope, optional broadcast address, flags (if present), and protocol description.
 *
 * @param o Output stream to write to.
 * @param a Address object to represent.
 * @return Reference to the output stream.
 */
auto operator<<(std::ostream& o, const Address& a) -> std::ostream&
{
    o << a.family() << " " << a.ip() << "/" << static_cast<int>(a.prefixLength());
    o << " scope " << a.scope();
    if (a.broadcast().has_value()) {
        o << " brd " << a.broadcast().value();
    }
    // TODO: to readable
    if (a.flags() != 0U) {
        o << " f 0x" << std::hex << a.flags() << std::dec;
    }
    switch (a.proto()) {
        case IFAPROT_UNSPEC:
            break;
        case IFAPROT_KERNEL_LO:
            o << " kernel_lo";
            break;
        case IFAPROT_KERNEL_RA:
            o << " kernel_ra";
            break;
        case IFAPROT_KERNEL_LL:
            o << " kernel_ll";
            break;
        default:
            o << " prot unknown: " << static_cast<int>(a.proto());
            break;
    }

    return o;
}

}  // namespace monkas::network
