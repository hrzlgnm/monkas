#pragma once

#include <bitset>
#include <chrono>
#include <optional>
#include <set>
#include <string>

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
        LowerUp,
        Dormant,
        Echo,
        // NOTE: keep last
        FlagsCount,
    };
    using LinkFlags = util::FlagSet<LinkFlag>;

    enum class ChangedFlag : uint8_t
    {
        Name,
        LinkFlags,
        OperationalState,
        MacAddress,
        BroadcastAddress,
        GatewayAddress,
        NetworkAddresses,
        // NOTE: keep last
        FlagsCount,
    };
    using ChangedFlags = util::FlagSet<ChangedFlag>;

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
    [[nodiscard]] auto linkFlags() const -> const LinkFlags&;

    [[nodiscard]] auto age() const -> Duration;

    [[nodiscard]] auto hasName() const -> bool;

    [[nodiscard]] auto hasChanges() const -> bool;
    [[nodiscard]] auto isChanged(ChangedFlag flag) const -> bool;
    [[nodiscard]] auto changedFlags() const -> const ChangedFlags&;

    void clearFlag(ChangedFlag flag);
    void clearChangedFlags();
    void logNerdstats() const;

  private:
    void touch(ChangedFlag flag);

    std::string m_name;
    ethernet::Address m_macAddress;
    ethernet::Address m_broadcastAddress;
    OperationalState m_operationalState {OperationalState::Unknown};
    Addresses m_networkAddresses;
    std::optional<ip::Address> m_gateway;
    std::chrono::time_point<std::chrono::steady_clock> m_lastChanged;
    ChangedFlags m_changedFlags;
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
        uint64_t changedFlagChanges {0};
        uint64_t changedFlagChecks {0};
        uint64_t changedFlagClears {0};
    } m_nerdstats;
};

using OperationalState = NetworkInterfaceStatusTracker::OperationalState;
auto operator<<(std::ostream& o, OperationalState op) -> std::ostream&;
using GatewayClearReason = NetworkInterfaceStatusTracker::GatewayClearReason;
auto operator<<(std::ostream& o, GatewayClearReason r) -> std::ostream&;
using ChangedFlag = NetworkInterfaceStatusTracker::ChangedFlag;
using ChangedFlags = NetworkInterfaceStatusTracker::ChangedFlags;
auto operator<<(std::ostream& o, ChangedFlag c) -> std::ostream&;
auto operator<<(std::ostream& o, const ChangedFlags& c) -> std::ostream&;
using LinkFlag = NetworkInterfaceStatusTracker::LinkFlag;
using LinkFlags = NetworkInterfaceStatusTracker::LinkFlags;

auto operator<<(std::ostream& o, LinkFlag l) -> std::ostream&;
auto operator<<(std::ostream& o, const LinkFlags& l) -> std::ostream&;

auto operator<<(std::ostream& o, const NetworkInterfaceStatusTracker& s) -> std::ostream&;

}  // namespace monkas::monitor

template<>
struct fmt::formatter<monkas::monitor::NetworkInterfaceStatusTracker> : ostream_formatter
{
};

template<>
struct fmt::formatter<monkas::monitor::OperationalState> : ostream_formatter
{
};

template<>
struct fmt::formatter<monkas::monitor::GatewayClearReason> : ostream_formatter
{
};

template<>
struct fmt::formatter<monkas::monitor::ChangedFlag> : ostream_formatter
{
};

template<>
struct fmt::formatter<monkas::monitor::ChangedFlags> : ostream_formatter
{
};

template<>
struct fmt::formatter<monkas::monitor::LinkFlag> : ostream_formatter
{
};

template<>
struct fmt::formatter<monkas::monitor::NetworkInterfaceStatusTracker::LinkFlags> : ostream_formatter
{
};
