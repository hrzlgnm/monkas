#include "NetworkInterfaceStatusTracker.h"
#include "ip/Address.h"
#include "network/NetworkAddress.h"

#include <spdlog/spdlog.h>
#include <string_view>

namespace monkas
{
template <typename T>
void logTrace(const T &t, NetworkInterfaceStatusTracker *that, const std::string_view &description)
{
    spdlog::trace("{} {}: {}", static_cast<void *>(that), description, t);
}

NetworkInterfaceStatusTracker::NetworkInterfaceStatusTracker() : m_lastChanged(std::chrono::steady_clock::now())
{
}

bool NetworkInterfaceStatusTracker::hasName() const
{
    return !m_name.empty();
}

void NetworkInterfaceStatusTracker::touch()
{
    m_lastChanged = std::chrono::steady_clock::now();
}

const std::string &NetworkInterfaceStatusTracker::name() const
{
    return m_name;
}

void NetworkInterfaceStatusTracker::setName(const std::string &name)
{
    touch();
    if (m_name != name)
    {
        m_name = name;
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
    touch();
    if (m_operState != operstate)
    {
        m_lastChanged = std::chrono::steady_clock::now();
        m_operState = operstate;
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
    touch();
    if (m_ethernetAddress != address)
    {
        m_ethernetAddress = address;
        logTrace(address, this, "ethernet address changed to");
    }
}

void NetworkInterfaceStatusTracker::setBroadcastAddress(const ethernet::Address &address)
{
    touch();
    if (m_broadcastAddress != address)
    {
        m_broadcastAddress = address;
        logTrace(address, this, "broadcast address changed to");
    }
}

const ip::Address &NetworkInterfaceStatusTracker::gatewayAddress() const
{
    return m_gateway;
}

void NetworkInterfaceStatusTracker::setGatewayAddress(const ip::Address &gateway)
{
    touch();
    if (m_gateway != gateway)
    {
        m_gateway = gateway;
        logTrace(gateway, this, "gateway address changed to");
    }
}

void NetworkInterfaceStatusTracker::clearGatewayAddress(GatewayClearReason r)
{
    if (m_gateway)
    {
        m_gateway = ip::Address();
        logTrace(r, this, "gateway cleared due to");
    }
}

std::set<network::NetworkAddress> NetworkInterfaceStatusTracker::networkAddresses() const
{
    return m_networkAddresses;
}

void NetworkInterfaceStatusTracker::addNetworkAddress(const network::NetworkAddress &address)
{
    touch();
    if (address.ip())
    {
        bool isNew = m_networkAddresses.erase(address) == 0;
        m_networkAddresses.insert(address);
        if (isNew)
        {
            logTrace(address, this, "address added");
        }
        else
        {
            logTrace(address, this, "address updated");
        }
    }
}

void NetworkInterfaceStatusTracker::removeNetworkAddress(const network::NetworkAddress &address)
{
    touch();
    auto res = m_networkAddresses.erase(address);
    if (res > 0)
    {
        logTrace(address, this, "address removed");
        if (std::count_if(std::begin(m_networkAddresses), std::end(m_networkAddresses),
                          [](const network::NetworkAddress &a) -> bool {
                              return a.adressFamily() == ip::AddressFamily::IPv4;
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
        // fallthrough
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
    return o;
}
} // namespace monkas
