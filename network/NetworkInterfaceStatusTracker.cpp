#include "NetworkInterfaceStatusTracker.h"

#include <spdlog/spdlog.h>
#include <string_view>
namespace monkas
{
template <typename T>
void logChange(const T &t, NetworkInterfaceStatusTracker *that, const std::string_view &description)
{
    spdlog::trace("{} {} changed to: {}", (void *)that, description, t);
}

NetworkInterfaceStatusTracker::NetworkInterfaceStatusTracker() : m_lastChanged(std::chrono::steady_clock::now())
{
}

int NetworkInterfaceStatusTracker::index() const
{
    return m_index;
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
        logChange(name, this, "name");
    }
}

void NetworkInterfaceStatusTracker::setIndex(int index)
{
    if (m_index != index)
    {
        touch();
        m_index = index;
        logChange(index, this, "index");
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
        logChange(operstate, this, "operational state");
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
        logChange(address, this, "ethernet address");
    }
}

void NetworkInterfaceStatusTracker::setBroadcastAddress(const ethernet::Address &address)
{
    touch();
    if (m_broadcastAddress != address)
    {
        m_broadcastAddress = address;
        logChange(address, this, "broadcast address");
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
        logChange(gateway, this, "gateway address");
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
            spdlog::trace("{} {} addr added: {}", (void *)this, m_name, address);
        }
        else
        {
            spdlog::trace("{} {} addr updated: {}", (void *)this, m_name, address);
        }
    }
}

void NetworkInterfaceStatusTracker::removeNetworkAddress(const network::NetworkAddress &address)
{
    touch();
    auto res = m_networkAddresses.erase(address);
    if (res > 0)
    {
        spdlog::trace("{} {} addr removed: {}", (void *)this, m_name, address);
    }
    else
    {
        spdlog::trace("{} {} addr not known: {}", (void *)this, m_name, address);
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
        return "Uknown";
    }
}

std::ostream &operator<<(std::ostream &o, OperationalState op)
{
    return o << to_string(op);
}
std::ostream &operator<<(std::ostream &o, const NetworkInterfaceStatusTracker &s)
{
    o << s.index() << ": " << s.name();
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
