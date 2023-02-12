#ifndef NETWORKINTERFACE_H
#define NETWORKINTERFACE_H

#include "ethernet/Address.h"
#include "network/NetworkAddress.h"
#include <chrono>
#include <set>
#include <string>

namespace monkas
{
using Duration = std::chrono::duration<int64_t, std::milli>;

// TODO: address struct with family, address, prefix length
// ideas:
// ethernet: Address: public std::array<uint8_t, 6>;
// ip: Ipv4Address: public std::array<uint8_t, 4>;
//     Ipv6Address: public std::array<uint8_t, 16>;
// TODO: stats for nerds per interface descriptor
class NetworkInterface
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

    NetworkInterface();

    int index() const;
    void setIndex(int index);

    const std::string &name() const;
    void setName(const std::string &name);

    OperationalState operationalState() const;
    void setOperationalState(OperationalState operstate);

    const ethernet::Address &ethernetAddress() const;
    void setEthernetAddress(const ethernet::Address &address);

    const ethernet::Address &broadcastAddress() const;
    void setBroadcastAddress(const ethernet::Address &address);

    const ip::Address &gatewayAddress() const;
    void setGatewayAddress(const ip::Address &gateway);

    std::set<network::NetworkAddress> addresses() const;

    void addAddress(const network::NetworkAddress &address);
    void delAddress(const network::NetworkAddress &address);

    Duration age() const;
    bool hasName() const;

  private:
    void touch();
    // TODO: only use public api
    friend std::ostream &operator<<(std::ostream &o, const NetworkInterface &s);
    int m_index{};
    std::string m_name;
    ethernet::Address m_ethernetAddress;
    ethernet::Address m_broadcastAddress;
    OperationalState m_operState{OperationalState::Unknown};
    std::set<network::NetworkAddress> m_addresses;
    ip::Address m_gateway;
    std::chrono::time_point<std::chrono::steady_clock> m_lastChanged;
};
using OperationalState = NetworkInterface::OperationalState;
} // namespace monkas
#endif
