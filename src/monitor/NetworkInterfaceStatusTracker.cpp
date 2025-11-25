#include <algorithm>
#include <ostream>
#include <string_view>
#include <utility>

#include <ip/Address.hpp>
#include <monitor/NetworkInterfaceStatusTracker.hpp>
#include <network/Address.hpp>
#include <spdlog/spdlog.h>

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

void NetworkInterfaceStatusTracker::touch(const ChangedFlag flag)
{
    if (!m_changedFlags.test(flag)) {
        m_lastChanged = std::chrono::steady_clock::now();
        m_changedFlags.set(flag);
        m_nerdstats.changedFlagChanges++;
        logTrace(flag, this, "change flag set");
    } else {
        logTrace(flag, this, "change flag already set");
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
        touch(ChangedFlag::Name);
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
        touch(ChangedFlag::OperationalState);
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
        touch(ChangedFlag::MacAddress);
        logTrace(address, this, "mac address changed to");
        m_nerdstats.macAddressChanges++;
    }
}

void NetworkInterfaceStatusTracker::setBroadcastAddress(const ethernet::Address& address)
{
    if (m_broadcastAddress != address || address.allZeroes()) {
        m_broadcastAddress = address;
        touch(ChangedFlag::BroadcastAddress);
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
        touch(ChangedFlag::GatewayAddress);
        logTrace(gateway, this, "gateway address changed to");
        m_nerdstats.gatewayAddressChanges++;
    }
}

void NetworkInterfaceStatusTracker::clearGatewayAddress(const GatewayClearReason r)
{
    if (m_gateway) {
        m_gateway = ip::Address();
        touch(ChangedFlag::GatewayAddress);
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
    if (m_networkAddresses.erase(address) == 0) {
        m_networkAddresses.insert(address);
        touch(ChangedFlag::NetworkAddresses);
        logTrace(address, this, "address added");
        m_nerdstats.networkAddressesAdded++;
    } else {
        // No material change – keep ordering stable, skip change-flag spam
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
        touch(ChangedFlag::NetworkAddresses);
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
        touch(ChangedFlag::LinkFlags);
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

auto NetworkInterfaceStatusTracker::hasChanges() const -> bool
{
    m_nerdstats.changedFlagChecks++;
    return m_changedFlags.any();
}

auto NetworkInterfaceStatusTracker::isChanged(const ChangedFlag flag) const -> bool
{
    m_nerdstats.changedFlagChecks++;
    return m_changedFlags.test(flag);
}

auto NetworkInterfaceStatusTracker::changedFlags() const -> ChangedFlags
{
    return m_changedFlags;
}

void NetworkInterfaceStatusTracker::clearFlag(const ChangedFlag flag)
{
    if (m_changedFlags.test(flag)) {
        m_changedFlags.reset(flag);
        m_nerdstats.changedFlagClears++;
        logTrace(flag, this, "change flag cleared");
    } else {
        logTrace(flag, this, "change flag already cleared");
    }
}

void NetworkInterfaceStatusTracker::clearChangedFlags()
{
    m_nerdstats.changedFlagClears += m_changedFlags.count();
    m_changedFlags.reset();
    logTrace("all change flags", this, "cleared");
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
    spdlog::info("changedFlag changes                  {}", m_nerdstats.changedFlagChanges);
    spdlog::info("changedFlag checks                   {}", m_nerdstats.changedFlagChecks);
    spdlog::info("changedFlag clears                   {}", m_nerdstats.changedFlagClears);
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
            return "Unknown";
    }
    return fmt::format("Unknown OperationalState({:02x})", std::to_underlying(o));
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

auto operator<<(std::ostream& o, ChangedFlag c) -> std::ostream&
{
    using enum NetworkInterfaceStatusTracker::ChangedFlag;
    switch (c) {
        case Name:
            return o << "NameChanged";
        case LinkFlags:
            return o << "LinkFlagsChanged";
        case OperationalState:
            return o << "OperationalStateChanged";
        case MacAddress:
            return o << "MacAddressChanged";
        case BroadcastAddress:
            return o << "BroadcastAddressChanged";
        case GatewayAddress:
            return o << "GatewayAddressChanged";
        case NetworkAddresses:
            return o << "NetworkAddressesChanged";
        case FlagsCount:
            break;
    }
    return o << fmt::format("Unknown ChangedFlag: 0x{:02x}", std::to_underlying(c));
}

auto operator<<(std::ostream& o, const ChangedFlags& c) -> std::ostream&
{
    return o << c.toString();
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
            break;
    }
    return o << fmt::format("Unknown LinkFlag: 0x{:02x}", std::to_underlying(l));
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
    o << " changed " << s.changedFlags();
    return o;
}

}  // namespace monkas::monitor
