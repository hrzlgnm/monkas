#pragma once

#include <chrono>
#include <ethernet/Address.hpp>
#include <fmt/ostream.h>
#include <network/NetworkAddress.hpp>
#include <set>
#include <string>

namespace monkas
{
using Duration = std::chrono::duration<int64_t, std::milli>;

// TODO: stats for nerds per interface descriptor
class NetworkInterfaceStatusTracker
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
    enum class GatewayClearReason : uint8_t
    {
        LinkDown,
        RouteDeleted,
        AllIPv4AddressesRemoved,
    };

    NetworkInterfaceStatusTracker();

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
    void clearGatewayAddress(GatewayClearReason r);

    std::set<network::NetworkAddress> networkAddresses() const;

    void addNetworkAddress(const network::NetworkAddress &address);
    void removeNetworkAddress(const network::NetworkAddress &address);

    Duration age() const;

    bool hasName() const;

  private:
    void touch();

    // TODO: only use public api
    friend std::ostream &operator<<(std::ostream &o, const NetworkInterfaceStatusTracker &s);
    std::string m_name;
    ethernet::Address m_ethernetAddress;
    ethernet::Address m_broadcastAddress;
    OperationalState m_operState{OperationalState::Unknown};
    std::set<network::NetworkAddress> m_networkAddresses;
    ip::Address m_gateway;
    std::chrono::time_point<std::chrono::steady_clock> m_lastChanged;
};

using OperationalState = NetworkInterfaceStatusTracker::OperationalState;
std::ostream &operator<<(std::ostream &o, OperationalState op);
using GatewayClearReason = NetworkInterfaceStatusTracker::GatewayClearReason;
std::ostream &operator<<(std::ostream &o, GatewayClearReason r);

} // namespace monkas

template <> struct fmt::formatter<monkas::NetworkInterfaceStatusTracker> : fmt::ostream_formatter
{
};

template <> struct fmt::formatter<monkas::OperationalState> : fmt::ostream_formatter
{
};

template <> struct fmt::formatter<monkas::GatewayClearReason> : fmt::ostream_formatter
{
};
