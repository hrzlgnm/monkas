#ifndef NETWORKINTERFACEDESCRIPTOR_H
#define NETWORKINTERFACEDESCRIPTOR_H

#include <chrono>
#include <string>
#include <unordered_set>

using Duration = std::chrono::duration<int64_t, std::milli>;

// TODO: address struct with family, address, prefix length
// TODO: stats for nerds per interface descriptor
class NetworkInterfaceDescriptor
{
  public:
    enum class OperationalState : uint8_t
    {
        Unknown,
        NotPresent,
        Down,
        LowerLayerDown,
        Testing,
        Dormant,
        Up,
    };

    NetworkInterfaceDescriptor();

    int index() const;
    const std::string &name() const;

    void setName(const std::string &name);
    void setIndex(int index);
    OperationalState operationalState() const;
    void setOperationalState(OperationalState operstate);
    std::string hardwareAddress() const;
    void setHardwareAddress(const std::string &hwaddr);
    void addAddress(const std::string &addr);
    void delAddress(const std::string &addr);
    Duration age() const;
    bool hasName() const;

  private:
    void touch();
    // TODO: only use public api
    friend std::ostream &operator<<(std::ostream &o, const NetworkInterfaceDescriptor &s);
    int m_index{};
    std::string m_name;
    std::string m_hardwareAddress;
    OperationalState m_operState{OperationalState::Unknown};
    std::unordered_set<std::string> m_addresses;
    std::chrono::time_point<std::chrono::steady_clock> m_lastChanged;
};
using OperationalState = NetworkInterfaceDescriptor::OperationalState;

#endif
