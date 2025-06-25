#pragma once

#include <bitset>
#include <chrono>
#include <optional>
#include <set>
#include <string>
#include <type_traits>

#include <ethernet/Address.hpp>
#include <fmt/ostream.h>
#include <network/Address.hpp>
#include <util/FlagSet.hpp>

namespace monkas::monitor
{

using Duration = std::chrono::duration<int64_t, std::milli>;
using Addresses = std::set<network::Address>;

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

    enum class LinkFlag : uint8_t
    {
        Up,
        Broadcast,
        Debug,
        Loopback,
        PointToPoint,
        NoTrailers,
        Running,
        NoArp,
        Promiscuous,
        AllMulticast,
        Master,
        Slave,
        Multicast,
        PortSet,
        AutoMedia,
        Dynamic,
        // NOTE: keep last
        FlagsCount,
    };
    using LinkFlags = util::FlagSet<LinkFlag>;

    enum class DirtyFlag : uint8_t
    {
        NameChanged,
        LinkFlagsChanged,
        OperationalStateChanged,
        MacAddressChanged,
        BroadcastAddressChanged,
        GatewayAddressChanged,
        NetworkAddressesChanged,
        // NOTE: keep last
        FlagsCount,
    };
    using DirtyFlags = util::FlagSet<DirtyFlag>;

    NetworkInterfaceStatusTracker();

    [[nodiscard]] auto name() const -> const std::string&;
    void setName(const std::string& name);

    [[nodiscard]] auto operationalState() const -> OperationalState;
    void setOperationalState(OperationalState operationalState);

    [[nodiscard]] auto macAddress() const -> const ethernet::Address&;
    void setMacAddress(const ethernet::Address& address);

    [[nodiscard]] auto broadcastAddress() const -> const ethernet::Address&;
    void setBroadcastAddress(const ethernet::Address& address);

    [[nodiscard]] auto gatewayAddress() const -> std::optional<ip::Address>;
    void setGatewayAddress(const ip::Address& gateway);
    void clearGatewayAddress(GatewayClearReason r);

    [[nodiscard]] auto networkAddresses() const -> const Addresses&;

    void addNetworkAddress(const network::Address& address);
    void removeNetworkAddress(const network::Address& address);

    void updateLinkFlags(const LinkFlags& flags);
    [[nodiscard]] auto linkFlags() const -> LinkFlags;

    [[nodiscard]] auto age() const -> Duration;

    [[nodiscard]] auto hasName() const -> bool;

    [[nodiscard]] auto isDirty() const -> bool;
    [[nodiscard]] auto isDirty(DirtyFlag flag) const -> bool;
    [[nodiscard]] auto dirtyFlags() const -> DirtyFlags;

    void clearFlag(DirtyFlag flag);
    void logNerdstats() const;

  private:
    void touch(DirtyFlag flag);

    std::string m_name;
    ethernet::Address m_macAddress;
    ethernet::Address m_broadcastAddress;
    OperationalState m_operationalState {OperationalState::Unknown};
    Addresses m_networkAddresses;
    std::optional<ip::Address> m_gateway;
    std::chrono::time_point<std::chrono::steady_clock> m_lastChanged;
    DirtyFlags m_dirtyFlags;
    LinkFlags m_linkFlags;

    // mutable for tracking const getters
    mutable struct Nerdstats
    {
        uint64_t nameChanges {0};
        uint64_t linkFlagChanges {0};
        uint64_t operationalStateChanges {0};
        uint64_t macAddressChanges {0};
        uint64_t broadcastAddressChanges {0};
        uint64_t gatewayAddressChanges {0};
        uint64_t gatewayAddressClears {0};
        uint64_t networkAddressesNoChangeUpdates {0};
        uint64_t networkAddressesAdded {0};
        uint64_t networkAddressesRemoved {0};
        uint64_t dirtyFlagChanges {0};
        uint64_t dirtyFlagChecks {0};
        uint64_t dirtyFlagClears {0};
    } m_nerdstats;
};

using OperationalState = NetworkInterfaceStatusTracker::OperationalState;
auto operator<<(std::ostream& o, OperationalState op) -> std::ostream&;
using GatewayClearReason = NetworkInterfaceStatusTracker::GatewayClearReason;
auto operator<<(std::ostream& o, GatewayClearReason r) -> std::ostream&;
using DirtyFlag = NetworkInterfaceStatusTracker::DirtyFlag;
using DirtyFlags = NetworkInterfaceStatusTracker::DirtyFlags;
auto operator<<(std::ostream& o, DirtyFlag d) -> std::ostream&;
auto operator<<(std::ostream& o, const DirtyFlags& d) -> std::ostream&;
using LinkFlag = NetworkInterfaceStatusTracker::LinkFlag;
using LinkFlags = NetworkInterfaceStatusTracker::LinkFlags;

auto operator<<(std::ostream& o, LinkFlag l) -> std::ostream&;
auto operator<<(std::ostream& o, const LinkFlags& l) -> std::ostream&;

auto operator<<(std::ostream& o, const NetworkInterfaceStatusTracker& s) -> std::ostream&;

}  // namespace monkas::monitor

template<>
struct fmt::formatter<monkas::monitor::NetworkInterfaceStatusTracker> : fmt::ostream_formatter
{
};

template<>
struct fmt::formatter<monkas::monitor::OperationalState> : fmt::ostream_formatter
{
};

template<>
struct fmt::formatter<monkas::monitor::GatewayClearReason> : fmt::ostream_formatter
{
};

template<>
struct fmt::formatter<monkas::monitor::DirtyFlag> : fmt::ostream_formatter
{
};

template<>
struct fmt::formatter<monkas::monitor::DirtyFlags> : fmt::ostream_formatter
{
};

template<>
struct fmt::formatter<monkas::monitor::LinkFlag> : fmt::ostream_formatter
{
};

template<>
struct fmt::formatter<monkas::monitor::NetworkInterfaceStatusTracker::LinkFlags> : fmt::ostream_formatter
{
};
