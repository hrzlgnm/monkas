#include <algorithm>
#include <ostream>
#include <sstream>
#include <string_view>

#include <fmt/format.h>
#include <ip/Address.hpp>
#include <monitor/NetworkInterfaceStatusTracker.hpp>
#include <network/Address.hpp>
#include <spdlog/spdlog.h>
#include <sys/types.h>

namespace monkas::monitor
{

namespace
{
template<typename T>
void logTrace(const T& t, NetworkInterfaceStatusTracker* that, const std::string_view& description)
{
    spdlog::trace("[{}][{}] {}: {}", static_cast<void*>(that), that->name(), description, t);
}
}  // namespace

NetworkInterfaceStatusTracker::NetworkInterfaceStatusTracker()
    : m_lastChanged(std::chrono::steady_clock::now())
{
}

auto NetworkInterfaceStatusTracker::hasName() const -> bool
{
    return !m_name.empty();
}

void NetworkInterfaceStatusTracker::touch(DirtyFlag flag)
{
    if (!m_dirtyFlags.test(flag)) {
        m_lastChanged = std::chrono::steady_clock::now();
        m_dirtyFlags.set(flag);
        m_nerdstats.dirtyFlagChanges++;
        logTrace(flag, this, "dirty flag set");
    } else {
        logTrace(flag, this, "dirty flag already set");
    }
}

auto NetworkInterfaceStatusTracker::name() const -> const std::string&
{
    return m_name;
}

void NetworkInterfaceStatusTracker::setName(const std::string& name)
{
    if (m_name != name) {
        m_name = name;
        touch(DirtyFlag::NameChanged);
        logTrace(name, this, "name changed to");
        m_nerdstats.nameChanges++;
    }
}

auto NetworkInterfaceStatusTracker::operationalState() const -> OperationalState
{
    return m_operationalState;
}

void NetworkInterfaceStatusTracker::setOperationalState(const OperationalState operationalState)
{
    if (m_operationalState != operationalState) {
        m_operationalState = operationalState;
        touch(DirtyFlag::OperationalStateChanged);
        logTrace(operationalState, this, "operational state changed to");
        m_nerdstats.operationalStateChanges++;
    }
}

auto NetworkInterfaceStatusTracker::macAddress() const -> const ethernet::Address&
{
    return m_macAddress;
}

auto NetworkInterfaceStatusTracker::broadcastAddress() const -> const ethernet::Address&
{
    return m_broadcastAddress;
}

void NetworkInterfaceStatusTracker::setMacAddress(const ethernet::Address& address)
{
    if (m_macAddress != address || address.allZeroes()) {
        m_macAddress = address;
        touch(DirtyFlag::MacAddressChanged);
        logTrace(address, this, "mac address changed to");
        m_nerdstats.macAddressChanges++;
    }
}

void NetworkInterfaceStatusTracker::setBroadcastAddress(const ethernet::Address& address)
{
    if (m_broadcastAddress != address || address.allZeroes()) {
        m_broadcastAddress = address;
        touch(DirtyFlag::BroadcastAddressChanged);
        logTrace(address, this, "broadcast address changed to");
        m_nerdstats.broadcastAddressChanges++;
    }
}

auto NetworkInterfaceStatusTracker::gatewayAddress() const -> std::optional<ip::Address>
{
    return m_gateway;
}

void NetworkInterfaceStatusTracker::setGatewayAddress(const ip::Address& gateway)
{
    if (m_gateway != gateway) {
        m_gateway = gateway;
        touch(DirtyFlag::GatewayAddressChanged);
        logTrace(gateway, this, "gateway address changed to");
        m_nerdstats.gatewayAddressChanges++;
    }
}

void NetworkInterfaceStatusTracker::clearGatewayAddress(GatewayClearReason r)
{
    if (m_gateway) {
        m_gateway = ip::Address();
        touch(DirtyFlag::GatewayAddressChanged);
        logTrace(r, this, "gateway cleared due to");
        m_nerdstats.gatewayAddressClears++;
    }
}

auto NetworkInterfaceStatusTracker::networkAddresses() const -> const Addresses&
{
    return m_networkAddresses;
}

void NetworkInterfaceStatusTracker::addNetworkAddress(const network::Address& address)
{
    if (const bool isNew = m_networkAddresses.erase(address) == 0) {
        m_networkAddresses.insert(address);
        touch(DirtyFlag::NetworkAddressesChanged);
        logTrace(address, this, "address added");
        m_nerdstats.networkAddressesAdded++;
    } else {
        // No material change – keep ordering stable, skip dirty-flag spam
        m_networkAddresses.insert(address);
        logTrace(address, this, "address unchanged");
        m_nerdstats.networkAddressesNoChangeUpdates++;
    }
}

void NetworkInterfaceStatusTracker::removeNetworkAddress(const network::Address& address)
{
    const auto res = m_networkAddresses.erase(address);
    m_nerdstats.networkAddressesRemoved += res;
    if (res > 0) {
        logTrace(address, this, "address removed");
        touch(DirtyFlag::NetworkAddressesChanged);
        if (std::ranges::count_if(m_networkAddresses,
                                  [](const network::Address& a) -> bool { return a.family() == ip::Family::IPv4; })
            == 0)
        {
            clearGatewayAddress(GatewayClearReason::AllIPv4AddressesRemoved);
        }
    } else {
        logTrace(address, this, "address unknown");
    }
}

void NetworkInterfaceStatusTracker::updateLinkFlags(const LinkFlags& flags)
{
    if (m_linkFlags != flags) {
        m_linkFlags = flags;
        touch(DirtyFlag::LinkFlagsChanged);
        logTrace(flags, this, "link flags updated to");
        m_nerdstats.linkFlagChanges++;
    }
}

auto NetworkInterfaceStatusTracker::linkFlags() const -> LinkFlags
{
    return m_linkFlags;
}

auto NetworkInterfaceStatusTracker::age() const -> Duration
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_lastChanged);
}

auto NetworkInterfaceStatusTracker::isDirty() const -> bool
{
    m_nerdstats.dirtyFlagChecks++;
    return m_dirtyFlags.any();
}

auto NetworkInterfaceStatusTracker::isDirty(DirtyFlag flag) const -> bool
{
    m_nerdstats.dirtyFlagChecks++;
    return m_dirtyFlags.test(flag);
}

auto NetworkInterfaceStatusTracker::dirtyFlags() const -> DirtyFlags
{
    return m_dirtyFlags;
}

void NetworkInterfaceStatusTracker::clearFlag(DirtyFlag flag)
{
    if (m_dirtyFlags.test(flag)) {
        m_dirtyFlags.reset(flag);
        m_nerdstats.dirtyFlagClears++;
        logTrace(flag, this, "dirty flag cleared");
    } else {
        logTrace(flag, this, "dirty flag already cleared");
    }
}

void NetworkInterfaceStatusTracker::logNerdstats() const
{
    spdlog::info("{:-^38}", m_name);
    spdlog::info("name changes                         {}", m_nerdstats.nameChanges);
    spdlog::info("LinkFlag changes                     {}", m_nerdstats.linkFlagChanges);
    spdlog::info("operationalState changes             {}", m_nerdstats.operationalStateChanges);
    spdlog::info("macAddress changes                   {}", m_nerdstats.macAddressChanges);
    spdlog::info("broadcastAddress changes             {}", m_nerdstats.broadcastAddressChanges);
    spdlog::info("gatewayAddress changes               {}", m_nerdstats.gatewayAddressChanges);
    spdlog::info("gatewayAddress clears                {}", m_nerdstats.gatewayAddressClears);
    spdlog::info("networkAddresses no change updates   {}", m_nerdstats.networkAddressesNoChangeUpdates);
    spdlog::info("networkAddresses added               {}", m_nerdstats.networkAddressesAdded);
    spdlog::info("networkAddresses removed             {}", m_nerdstats.networkAddressesRemoved);
    spdlog::info("dirtyFlag changes                    {}", m_nerdstats.dirtyFlagChanges);
    spdlog::info("dirtyFlag checks                     {}", m_nerdstats.dirtyFlagChecks);
    spdlog::info("dirtyFlag clears                     {}", m_nerdstats.dirtyFlagClears);
    spdlog::info("{:-^38}", "-");
}

namespace
{
auto toString(const NetworkInterfaceStatusTracker::OperationalState o) -> std::string
{
    using enum NetworkInterfaceStatusTracker::OperationalState;
    switch (o) {
        case NotPresent:
            return "NotPresent";
        case Down:
            return "Down";
        case LowerLayerDown:
            return "LowerLayerDown";
        case Testing:
            return "Testing";
        case Dormant:
            return "Dormant";
        case Up:
            return "Up";
        case Unknown:
        default:
            return "Unknown";
    }
}

}  // namespace

auto operator<<(std::ostream& o, const OperationalState op) -> std::ostream&
{
    return o << toString(op);
}

auto operator<<(std::ostream& o, const GatewayClearReason r) -> std::ostream&
{
    using enum NetworkInterfaceStatusTracker::GatewayClearReason;
    switch (r) {
        case LinkDown:
            o << "LinkDown";
            break;
        case RouteDeleted:
            o << "RouteDeleted";
            break;
        case AllIPv4AddressesRemoved:
            o << "AllIPv4AddressesRemoved";
            break;
    }
    return o;
}

auto operator<<(std::ostream& o, DirtyFlag d) -> std::ostream&
{
    using enum NetworkInterfaceStatusTracker::DirtyFlag;
    switch (d) {
        case NameChanged:
            return o << "NameChanged";
        case LinkFlagsChanged:
            return o << "LinkFlagsChanged";
        case OperationalStateChanged:
            return o << "OperationalStateChanged";
        case MacAddressChanged:
            return o << "MacAddressChanged";
        case BroadcastAddressChanged:
            return o << "BroadcastAddressChanged";
        case GatewayAddressChanged:
            return o << "GatewayAddressChanged";
        case NetworkAddressesChanged:
            return o << "NetworkAddressesChanged";
        case FlagsCount:
        default:
            return o << "UnknownDirtyFlag: 0x" << std::hex << static_cast<uint8_t>(d);
    }
}

auto operator<<(std::ostream& o, const DirtyFlags& d) -> std::ostream&
{
    return o << d.toString();
}

auto operator<<(std::ostream& o, NetworkInterfaceStatusTracker::LinkFlag l) -> std::ostream&
{
    using enum NetworkInterfaceStatusTracker::LinkFlag;
    switch (l) {
        case Up:
            return o << "Up";
        case Broadcast:
            return o << "Broadcast";
        case Debug:
            return o << "Debug";
        case Loopback:
            return o << "Loopback";
        case PointToPoint:
            return o << "PointToPoint";
        case NoTrailers:
            return o << "NoTrailers";
        case Running:
            return o << "Running";
        case NoArp:
            return o << "NoArp";
        case Promiscuous:
            return o << "Promiscuous";
        case AllMulticast:
            return o << "AllMulticast";
        case Master:
            return o << "Master";
        case Slave:
            return o << "Slave";
        case Multicast:
            return o << "Multicast";
        case PortSet:
            return o << "PortSet";
        case AutoMedia:
            return o << "AutoMedia";
        case Dynamic:
            return o << "Dynamic";
        case LowerUp:
            return o << "LowerUp";
        case Dormant:
            return o << "Dormant";
        case Echo:
            return o << "Echo";
        case FlagsCount:
        default:
            return o << "UnknownLinkFlag: 0x" << std::hex << static_cast<uint8_t>(l);
    }
}

auto operator<<(std::ostream& o, const LinkFlags& l) -> std::ostream&

{
    return o << '<' << l.toString() << '>';
}

auto operator<<(std::ostream& o, const NetworkInterfaceStatusTracker& s) -> std::ostream&
{
    o << s.name();
    o << " " << s.linkFlags();
    o << " mac " << s.macAddress();
    o << " brd " << s.broadcastAddress();
    if (!s.networkAddresses().empty()) {
        o << " [";
        bool first = true;
        for (const auto& a : s.networkAddresses()) {
            if (!first) {
                o << ", ";
            }
            o << a;
            first = false;
        }
        o << "]";
    }
    if (s.gatewayAddress().has_value()) {
        o << " default via " << s.gatewayAddress().value();
    }
    o << " op " << toString(s.operationalState()) << "(" << static_cast<int>(s.operationalState()) << ")";
    o << " age " << s.age().count();
    o << " dirty " << s.dirtyFlags();
    return o;
}

}  // namespace monkas::monitor
