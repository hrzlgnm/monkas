#include "NetworkInterfaceDescriptor.h"

#include <glog/logging.h>

NetworkInterfaceDescriptor::NetworkInterfaceDescriptor() : m_lastChanged(std::chrono::steady_clock::now())
{
}

int NetworkInterfaceDescriptor::index() const
{
    return m_index;
}

bool NetworkInterfaceDescriptor::hasName() const
{
    return !m_name.empty();
}

void NetworkInterfaceDescriptor::touch()
{
    m_lastChanged = std::chrono::steady_clock::now();
}

const std::string &NetworkInterfaceDescriptor::name() const
{
    return m_name;
}

void NetworkInterfaceDescriptor::setName(const std::string &name)
{
    touch();
    if (m_name != name)
    {
        m_name = name;
        VLOG(1) << this << " "
                << "name changed to: " << name << "\n";
    }
}

void NetworkInterfaceDescriptor::setIndex(int index)
{
    if (m_index != index)
    {
        touch();
        m_index = index;
        VLOG(1) << this << " "
                << "index changed to: " << index << "\n";
    }
}

NetworkInterfaceDescriptor::OperationalState NetworkInterfaceDescriptor::operationalState() const
{
    return m_operState;
}

void NetworkInterfaceDescriptor::setOperationalState(OperationalState operstate)
{
    touch();
    if (m_operState != operstate)
    {
        m_lastChanged = std::chrono::steady_clock::now();
        m_operState = operstate;
        VLOG(1) << this << " " << m_name << " operstate changed to: " << static_cast<int>(operstate) << "\n";
    }
}

std::string NetworkInterfaceDescriptor::hardwareAddress() const
{
    return m_hardwareAddress;
}

void NetworkInterfaceDescriptor::setHardwareAddress(const std::string &hwaddr)
{
    touch();
    if (m_hardwareAddress != hwaddr)
    {
        m_hardwareAddress = hwaddr;
        VLOG(1) << this << " " << m_name << " hwaddr changed to: " << hwaddr << "\n";
    }
}

void NetworkInterfaceDescriptor::addAddress(const std::string &addr)
{
    touch();
    auto res = m_addresses.emplace(addr);
    if (res.second)
    {
        VLOG(1) << this << " " << m_name << " addr added: " << addr << "\n";
    }
    else
    {
        VLOG(1) << this << " " << m_name << " addr is already known: " << addr << "\n";
    }
}

void NetworkInterfaceDescriptor::delAddress(const std::string &addr)
{
    touch();
    auto res = m_addresses.erase(addr);
    if (res > 0)
    {
        VLOG(1) << this << " " << m_name << " addr removed: " << addr << "\n";
    }
    else
    {
        VLOG(1) << this << " " << m_name << " addr is not known: " << addr << "\n";
    }
}

Duration NetworkInterfaceDescriptor::age() const
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_lastChanged);
}

std::string to_string(NetworkInterfaceDescriptor::OperationalState o)
{
    using OperState = NetworkInterfaceDescriptor::OperationalState;
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

std::ostream &operator<<(std::ostream &o, const NetworkInterfaceDescriptor &s)
{
    o << s.index() << ": " << s.name();
    o << " hwaddr=" << s.m_hardwareAddress;
    for (const auto &a : s.m_addresses)
    {
        o << " " << a;
    }
    o << " state=" << to_string(s.m_operState) << "(" << static_cast<int>(s.m_operState) << ")";
    o << " age=" << s.age().count();
    return o;
}
