#pragma once

#include <bitset>
#include <chrono>
#include <ethernet/Address.hpp>
#include <fmt/ostream.h>
#include <network/NetworkAddress.hpp>
#include <set>
#include <string>
#include <type_traits>

namespace monkas
{
using Duration = std::chrono::duration<int64_t, std::milli>;
using NetworkAddresses = std::set<network::NetworkAddress>;

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

    enum class DirtyFlag : uint8_t
    {
        NameChanged,
        OperationalStateChanged,
        EthernetAddressChanged,
        BroadcastAddressChanged,
        GatewayAddressChanged,
        NetworkAddressesChanged,
        // NOTE: keep last
        FlagsCount,
    };

    using DirtyFlags = std::bitset<std::underlying_type_t<DirtyFlag>(DirtyFlag::FlagsCount)>;

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

    const NetworkAddresses &networkAddresses() const;

    void addNetworkAddress(const network::NetworkAddress &address);
    void removeNetworkAddress(const network::NetworkAddress &address);

    Duration age() const;

    bool hasName() const;

    bool isDirty() const;
    bool isDirty(DirtyFlag flag) const;
    DirtyFlags dirtyFlags() const;
    void clearFlag(DirtyFlag flag);

  private:
    void touch(DirtyFlag flag);

    // TODO: only use public api
    friend std::ostream &operator<<(std::ostream &o, const NetworkInterfaceStatusTracker &s);
    std::string m_name;
    ethernet::Address m_ethernetAddress;
    ethernet::Address m_broadcastAddress;
    OperationalState m_operState{OperationalState::Unknown};
    NetworkAddresses m_networkAddresses;
    ip::Address m_gateway;
    std::chrono::time_point<std::chrono::steady_clock> m_lastChanged;
    DirtyFlags m_dirtyFlags;
};

using OperationalState = NetworkInterfaceStatusTracker::OperationalState;
std::ostream &operator<<(std::ostream &o, OperationalState op);
using GatewayClearReason = NetworkInterfaceStatusTracker::GatewayClearReason;
std::ostream &operator<<(std::ostream &o, GatewayClearReason r);
using DirtyFlag = NetworkInterfaceStatusTracker::DirtyFlag;
using DirtyFlags = NetworkInterfaceStatusTracker::DirtyFlags;
std::ostream &operator<<(std::ostream &o, DirtyFlag d);
std::ostream &operator<<(std::ostream &o, DirtyFlags d);

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

template <> struct fmt::formatter<monkas::DirtyFlag> : fmt::ostream_formatter
{
};

template <> struct fmt::formatter<monkas::DirtyFlags> : fmt::ostream_formatter
{
};
