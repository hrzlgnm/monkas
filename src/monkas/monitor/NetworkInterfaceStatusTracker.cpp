#include <fmt/format.h>
#include <ip/Address.hpp>
#include <monitor/NetworkInterfaceStatusTracker.hpp>
#include <network/NetworkAddress.hpp>

#include <algorithm>
#include <ostream>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string_view>
#include <sys/types.h>
#include <type_traits>

namespace monkas
{
template <typename T>
void logTrace(const T &t, NetworkInterfaceStatusTracker *that, const std::string_view &description)
{
    spdlog::trace("[{}][{}] {}: {}", static_cast<void *>(that), that->name(), description, t);
}

NetworkInterfaceStatusTracker::NetworkInterfaceStatusTracker()
    : m_lastChanged(std::chrono::steady_clock::now())
{
}

bool NetworkInterfaceStatusTracker::hasName() const
{
    return !m_name.empty();
}

void NetworkInterfaceStatusTracker::touch(DirtyFlag flag)
{
    // TODO: use std::to_underlying with c++23
    const auto idx = fmt::underlying(flag);
    if (idx >= m_dirtyFlags.size())
    {
        spdlog::error("Invalid dirty flag index: {}", idx);
        return;
    }
    if (!m_dirtyFlags.test(idx))
    {
        m_lastChanged = std::chrono::steady_clock::now();
        m_dirtyFlags.set(idx);
        logTrace(flag, this, "dirty flag set");
    }
    else
    {
        logTrace(flag, this, "dirty flag already set");
    }
}

const std::string &NetworkInterfaceStatusTracker::name() const
{
    return m_name;
}

void NetworkInterfaceStatusTracker::setName(const std::string &name)
{
    if (m_name != name)
    {
        m_name = name;
        touch(DirtyFlag::NameChanged);
        logTrace(name, this, "name changed to");
    }
}

// TODO: fallback to flags
NetworkInterfaceStatusTracker::OperationalState NetworkInterfaceStatusTracker::operationalState() const
{
    return m_operState;
}

void NetworkInterfaceStatusTracker::setOperationalState(OperationalState operstate)
{
    if (m_operState != operstate)
    {
        m_operState = operstate;
        touch(DirtyFlag::OperationalStateChanged);
        logTrace(operstate, this, "operational state changed to");
    }
}

const ethernet::Address &NetworkInterfaceStatusTracker::ethernetAddress() const
{
    return m_ethernetAddress;
}

const ethernet::Address &NetworkInterfaceStatusTracker::broadcastAddress() const
{
    return m_broadcastAddress;
}

void NetworkInterfaceStatusTracker::setEthernetAddress(const ethernet::Address &address)
{
    if (m_ethernetAddress != address)
    {
        m_ethernetAddress = address;
        touch(DirtyFlag::EthernetAddressChanged);
        logTrace(address, this, "ethernet address changed to");
    }
}

void NetworkInterfaceStatusTracker::setBroadcastAddress(const ethernet::Address &address)
{
    if (m_broadcastAddress != address)
    {
        m_broadcastAddress = address;
        touch(DirtyFlag::BroadcastAddressChanged);
        logTrace(address, this, "broadcast address changed to");
    }
}

const ip::Address &NetworkInterfaceStatusTracker::gatewayAddress() const
{
    return m_gateway;
}

void NetworkInterfaceStatusTracker::setGatewayAddress(const ip::Address &gateway)
{
    if (m_gateway != gateway)
    {
        m_gateway = gateway;
        touch(DirtyFlag::GatewayAddressChanged);
        logTrace(gateway, this, "gateway address changed to");
    }
}

void NetworkInterfaceStatusTracker::clearGatewayAddress(GatewayClearReason r)
{
    if (m_gateway)
    {
        m_gateway = ip::Address();
        touch(DirtyFlag::GatewayAddressChanged);
        logTrace(r, this, "gateway cleared due to");
    }
}

NetworkAddresses NetworkInterfaceStatusTracker::networkAddresses() const
{
    return m_networkAddresses;
}

void NetworkInterfaceStatusTracker::addNetworkAddress(const network::NetworkAddress &address)
{
    if (address.ip())
    {
        const bool isNew = m_networkAddresses.erase(address) == 0;
        if (isNew)
        {
            m_networkAddresses.insert(address);
            touch(DirtyFlag::NetworkAddressesChanged);
            logTrace(address, this, "address added");
        }
        else
        {
            // No material change – keep ordering stable, skip dirty-flag spam
            m_networkAddresses.insert(address);
            logTrace(address, this, "address unchanged");
        }
    }
}

void NetworkInterfaceStatusTracker::removeNetworkAddress(const network::NetworkAddress &address)
{
    auto res = m_networkAddresses.erase(address);
    if (res > 0)
    {
        logTrace(address, this, "address removed");
        touch(DirtyFlag::NetworkAddressesChanged);
        if (std::count_if(std::begin(m_networkAddresses), std::end(m_networkAddresses),
                          [](const network::NetworkAddress &a) -> bool {
                              return a.addressFamily() == ip::AddressFamily::IPv4;
                          }) == 0)
        {
            clearGatewayAddress(GatewayClearReason::AllIPv4AddressesRemoved);
        }
    }
    else
    {
        logTrace(address, this, "address unknown");
    }
}

Duration NetworkInterfaceStatusTracker::age() const
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_lastChanged);
}

bool NetworkInterfaceStatusTracker::isDirty() const
{
    return m_dirtyFlags.any();
}

bool NetworkInterfaceStatusTracker::isDirty(DirtyFlag flag) const
{
    const auto idx = fmt::underlying(flag);
    if (idx >= m_dirtyFlags.size())
    {
        spdlog::error("Invalid dirty flag index: {}, returnig false", idx);
        return false;
    }
    return m_dirtyFlags.test(idx);
}

DirtyFlags NetworkInterfaceStatusTracker::dirtyFlags() const
{
    return m_dirtyFlags;
}

void NetworkInterfaceStatusTracker::clearFlag(DirtyFlag flag)
{
    // TODO: use std::to_underlying with c++23
    const auto idx = fmt::underlying(flag);
    if (idx >= m_dirtyFlags.size())
    {
        spdlog::error("Invalid dirty flag index: {}", idx);
        return;
    }
    if (m_dirtyFlags.test(idx))
    {
        m_dirtyFlags.reset(idx);
        logTrace(flag, this, "dirty flag cleared");
    }
    else
    {
        logTrace(flag, this, "dirty flag already cleared");
    }
}

std::string to_string(NetworkInterfaceStatusTracker::OperationalState o)
{
    using OperState = NetworkInterfaceStatusTracker::OperationalState;
    switch (o)
    {
    case OperState::NotPresent:
        return "NotPresent";
    case OperState::Down:
        return "Down";
    case OperState::LowerLayerDown:
        return "LowerLayerDown";
    case OperState::Testing:
        return "Testing";
    case OperState::Dormant:
        return "Dormant";
    case OperState::Up:
        return "Up";
    case OperState::Unknown:
    default:
        return "Unknown";
    }
}

std::ostream &operator<<(std::ostream &o, OperationalState op)
{
    return o << to_string(op);
}

std::ostream &operator<<(std::ostream &o, GatewayClearReason r)
{
    switch (r)
    {
    case GatewayClearReason::LinkDown:
        o << "LinkDown";
        break;
    case GatewayClearReason::RouteDeleted:
        o << "RouteDeleted";
        break;
    case GatewayClearReason::AllIPv4AddressesRemoved:
        o << "AllIPv4AddressesRemoved";
        break;
    }
    return o;
}

std::ostream &operator<<(std::ostream &o, DirtyFlag d)
{
    switch (d)
    {
    case NetworkInterfaceStatusTracker::DirtyFlag::NameChanged:
        return o << "NameChanged";
    case NetworkInterfaceStatusTracker::DirtyFlag::OperationalStateChanged:
        return o << "OperationalStateChanged";
    case NetworkInterfaceStatusTracker::DirtyFlag::EthernetAddressChanged:
        return o << "EthernetAddressChanged";
    case NetworkInterfaceStatusTracker::DirtyFlag::BroadcastAddressChanged:
        return o << "BroadcastAddressChanged";
    case NetworkInterfaceStatusTracker::DirtyFlag::GatewayAddressChanged:
        return o << "GatewayAddressChanged";
    case NetworkInterfaceStatusTracker::DirtyFlag::NetworkAddressesChanged:
        return o << "NetworkAddressesChanged";
    case NetworkInterfaceStatusTracker::DirtyFlag::FlagsCount:
    default:
        return o << "UnknownDirtyFlag: 0x" << std::hex << static_cast<uint8_t>(d);
    }
}

std::string dirtyFlagsToString(const DirtyFlags &flags)
{
    if (flags.none())
    {
        return "None";
    }

    std::ostringstream result;
    bool empty = true;

    for (std::underlying_type_t<DirtyFlag> i = 0; i < std::underlying_type_t<DirtyFlag>(DirtyFlag::FlagsCount); ++i)
    {
        if (flags.test(i))
        {
            if (!empty)
            {
                result << "|";
            }
            result << DirtyFlag(i);
            empty = false;
        }
    }

    return result.str();
}

std::ostream &operator<<(std::ostream &o, const DirtyFlags &d)
{
    return o << dirtyFlagsToString(d);
}

std::ostream &operator<<(std::ostream &o, const NetworkInterfaceStatusTracker &s)
{
    o << s.name();
    if (s.m_ethernetAddress)
    {
        o << " ether " << s.m_ethernetAddress;
    }
    if (s.m_broadcastAddress)
    {
        o << " brd " << s.m_broadcastAddress;
    }
    if (!s.m_networkAddresses.empty())
    {
        o << " [";
        bool first = true;
        for (const auto &a : s.m_networkAddresses)
        {
            if (!first)
            {
                o << ", ";
            }
            o << a;
            first = false;
        }
        o << "]";
    }
    if (s.m_gateway)
    {
        o << " default via " << s.m_gateway;
    }
    o << " op " << to_string(s.m_operState) << "(" << static_cast<int>(s.m_operState) << ")";
    o << " age " << s.age().count();
    o << " dirty " << dirtyFlagsToString(s.dirtyFlags());
    return o;
}
} // namespace monkas
