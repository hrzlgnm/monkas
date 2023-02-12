#include "NetworkInterface.h"

#include <glog/logging.h>
namespace monkas
{

NetworkInterface::NetworkInterface() : m_lastChanged(std::chrono::steady_clock::now())
{
}

int NetworkInterface::index() const
{
    return m_index;
}

bool NetworkInterface::hasName() const
{
    return !m_name.empty();
}

void NetworkInterface::touch()
{
    m_lastChanged = std::chrono::steady_clock::now();
}

const std::string &NetworkInterface::name() const
{
    return m_name;
}

void NetworkInterface::setName(const std::string &name)
{
    touch();
    if (m_name != name)
    {
        m_name = name;
        VLOG(1) << this << " "
                << "name changed to: " << name << "\n";
    }
}

void NetworkInterface::setIndex(int index)
{
    if (m_index != index)
    {
        touch();
        m_index = index;
        VLOG(1) << this << " "
                << "index changed to: " << index << "\n";
    }
}

NetworkInterface::OperationalState NetworkInterface::operationalState() const
{
    return m_operState;
}

void NetworkInterface::setOperationalState(OperationalState operstate)
{
    touch();
    if (m_operState != operstate)
    {
        m_lastChanged = std::chrono::steady_clock::now();
        m_operState = operstate;
        VLOG(1) << this << " " << m_name << " operstate changed to: " << static_cast<int>(operstate) << "\n";
    }
}

const ethernet::Address &NetworkInterface::ethernetAddress() const
{
    return m_ethernetAddress;
}

const ethernet::Address &NetworkInterface::broadcastAddress() const
{
    return m_broadcastAddress;
}

void NetworkInterface::setEthernetAddress(const ethernet::Address &address)
{
    touch();
    if (m_ethernetAddress != address)
    {
        m_ethernetAddress = address;
        VLOG(1) << this << " " << m_name << " ethernet address changed to: " << address << "\n";
    }
}

void NetworkInterface::setBroadcastAddress(const ethernet::Address &address)
{
    touch();
    if (m_broadcastAddress != address)
    {
        m_broadcastAddress = address;
        VLOG(1) << this << " " << m_name << " broadcast address changed to: " << address << "\n";
    }
}

const ip::Address &NetworkInterface::gatewayAddress() const
{
    return m_gateway;
}

void NetworkInterface::setGatewayAddress(const ip::Address &gateway)
{
    touch();
    if (m_gateway != gateway)
    {
        m_gateway = gateway;
        VLOG(1) << this << " " << m_name << " gateway changed to: " << gateway << "\n";
    }
}

std::set<network::NetworkAddress> NetworkInterface::addresses() const
{
    return m_addresses;
}

void NetworkInterface::addAddress(const network::NetworkAddress &address)
{
    touch();
    auto res = m_addresses.emplace(address);
    if (res.second)
    {
        VLOG(1) << this << " " << m_name << " addr added: " << address << "\n";
    }
    else
    {
        VLOG(1) << this << " " << m_name << " addr is already known: " << address << "\n";
    }
}

void NetworkInterface::delAddress(const network::NetworkAddress &address)
{
    touch();
    auto res = m_addresses.erase(address);
    if (res > 0)
    {
        VLOG(1) << this << " " << m_name << " addr removed: " << address << "\n";
    }
    else
    {
        VLOG(1) << this << " " << m_name << " addr is not known: " << address << "\n";
    }
}

Duration NetworkInterface::age() const
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_lastChanged);
}

std::string to_string(NetworkInterface::OperationalState o)
{
    using OperState = NetworkInterface::OperationalState;
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

std::ostream &operator<<(std::ostream &o, const NetworkInterface &s)
{
    o << s.index() << ": " << s.name();
    if (s.m_ethernetAddress)
    {
        o << " ethaddr=" << s.m_ethernetAddress;
    }
    if (s.m_broadcastAddress)
    {
        o << " bcast=" << s.m_broadcastAddress;
    }
    if (!s.m_addresses.empty())
    {
        o << " addrs=[";
        bool first = true;
        for (const auto &a : s.m_addresses)
        {
            if (!first)
            {
                o << ", ";
            }
            o << a;
            first = false;
        }
        o << " ]";
    }
    if (s.m_gateway)
    {
        o << " gateway=" << s.m_gateway;
    }
    o << " state=" << to_string(s.m_operState) << "(" << static_cast<int>(s.m_operState) << ")";
    o << " age=" << s.age().count();
    return o;
}
} // namespace monkas
