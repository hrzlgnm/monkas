#pragma once

#include "ip/Address.hpp"
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <monitor/NetworkInterfaceStatusTracker.hpp>
#include <network/Interface.hpp>
#include <optional>
#include <vector>
#include <watchable/Watchable.hpp>

// TODO: sometimes an enthernet interface comes up with Unknown operstate, ip link shows the same info, what do we want
// to do in this case?
// TODO: consider ENOBUFS errno from recv as resync point

struct mnl_socket;
struct nlmsghdr;
struct ifinfomsg;
struct ifaddrmsg;
struct rtmsg;
struct nlattr;

namespace monkas
{

// neat ADL trick to treat uint8_t as numbers
inline auto operator<<(std::ostream &os, uint8_t c) -> std::ostream &
{
    return os << static_cast<uint16_t>(c);
}

// TODO: wrap this into a own class with validating getters
using RtnlAttributes = std::vector<const nlattr *>;

enum RuntimeFlag : uint8_t
{
    StatsForNerds = 1,
    PreferredFamilyV4 = 2,
    PreferredFamilyV6 = 4,
    DumpPackets = 8,
    NonBlocking = 16,
};

enum class InitialSnapshotMode : uint8_t
{
    NoInitialSnapshot = 0,
    InitialSnapshot = 1,
};

// TODO make this a std::bitset, too
using RuntimeOptions = uint32_t;

using Interfaces = std::set<network::Interface>;

using InterfacesNotifier = Watchable<const Interfaces>;
using InterfacesWatcher = InterfacesNotifier::Watcher;
using InterfacesWatcherToken = InterfacesNotifier::Token;

using OperationalState = NetworkInterfaceStatusTracker::OperationalState;
using OperationalStateNotifier = Watchable<const network::Interface, OperationalState>;
using OperationalStateWatcher = OperationalStateNotifier::Watcher;
using OperationalStateWatcherToken = OperationalStateNotifier::Token;

using NetworkAddressNotifier = Watchable<const network::Interface, const NetworkAddresses>;
using NetworkAddressWatcher = NetworkAddressNotifier::Watcher;
using NetworkAddressWatcherToken = NetworkAddressNotifier::Token;

using GatewayAddressNotifier = Watchable<const network::Interface, const ip::Address>;
using GatewayAddressWatcher = GatewayAddressNotifier::Watcher;
using GatewayAddressWatcherToken = GatewayAddressNotifier::Token;

using MacAddressNotifier = Watchable<network::Interface, const ethernet::Address>;
using MacAddressWatcher = MacAddressNotifier::Watcher;
using MacAddressWatcherToken = MacAddressNotifier::Token;

using BroadcastAddressNotifier = Watchable<network::Interface, const ethernet::Address>;
using BroadcastAddressWatcher = BroadcastAddressNotifier::Watcher;
using BroadcastAddressWatcherToken = BroadcastAddressNotifier::Token;

using EnumerationDoneNotifier = Watchable<>;
using EnumerationDoneWatcher = EnumerationDoneNotifier::Watcher;
using EnumerationDoneWatcherToken = EnumerationDoneNotifier::Token;

class RtnlNetworkMonitor
{
  public:
    explicit RtnlNetworkMonitor(const RuntimeOptions &options);
    void enumerateInterfaces();
    auto run() -> int;
    void stop();

    [[nodiscard]] auto addInterfacesWatcher(
        const InterfacesWatcher &watcher, InitialSnapshotMode initialSnapshot = InitialSnapshotMode::NoInitialSnapshot)
        -> InterfacesWatcherToken;
    void removeInterfacesWatcher(const InterfacesWatcherToken &token);

    [[nodiscard]] auto addOperationalStateWatcher(
        const OperationalStateWatcher &watcher,
        InitialSnapshotMode initialSnapshot = InitialSnapshotMode::NoInitialSnapshot) -> OperationalStateWatcherToken;
    void removeOperationalStateWatcher(const OperationalStateWatcherToken &token);

    [[nodiscard]] auto addNetworkAddressWatcher(
        const NetworkAddressWatcher &watcher,
        InitialSnapshotMode initialSnapshot = InitialSnapshotMode::NoInitialSnapshot) -> NetworkAddressWatcherToken;
    void removeNetworkAddressWatcher(const NetworkAddressWatcherToken &token);

    [[nodiscard]] auto addGatewayAddressWatcher(
        const GatewayAddressWatcher &watcher,
        InitialSnapshotMode initialSnapshot = InitialSnapshotMode::NoInitialSnapshot) -> GatewayAddressWatcherToken;
    void removeGatewayAddressWatcher(const GatewayAddressWatcherToken &token);

    [[nodiscard]] auto addMacAddressWatcher(
        const MacAddressWatcher &watcher, InitialSnapshotMode initialSnapshot = InitialSnapshotMode::NoInitialSnapshot)
        -> MacAddressWatcherToken;
    void removeMacAddressWatcher(const MacAddressWatcherToken &token);

    [[nodiscard]] auto addBroadcastAddressWatcher(
        const MacAddressWatcher &watcher, InitialSnapshotMode initialSnapshot = InitialSnapshotMode::NoInitialSnapshot)
        -> BroadcastAddressWatcherToken;

    void removeBroadcastAddressWatcher(const BroadcastAddressWatcherToken &token);

    // enumeration done watcher is called when enumeration is done, or immediately if enumeration is already done
    // the optional return value is used to indicate that enumeration is already done
    [[nodiscard]] auto addEnumerationDoneWatcher(const EnumerationDoneWatcher &watcher)
        -> std::optional<EnumerationDoneWatcherToken>;
    void removeEnumerationDoneWatcher(const EnumerationDoneWatcherToken &token);

  private:
    void receiveAndProcess();

    /* @note: only one such request can be in progress until the reply is received */
    void sendDumpRequest(uint16_t msgType);

    void parseLinkMessage(const nlmsghdr *nlhdr, const ifinfomsg *ifi);
    void parseAddressMessage(const nlmsghdr *nlhdr, const ifaddrmsg *ifa);
    void parseRouteMessage(const nlmsghdr *nlhdr, const rtmsg *rtm);

    auto ensureNameCurrent(uint32_t ifIndex, const nlattr *nameAttribute) -> NetworkInterfaceStatusTracker &;

    void printStatsForNerdsIfEnabled();

    auto mnlMessageCallback(const nlmsghdr *n) -> int;
    static auto dispatchMnMessageCallbackToSelf(const struct nlmsghdr *n, void *self) -> int;

    auto parseAttributes(const nlmsghdr *n, size_t offset, uint16_t maxType) -> RtnlAttributes;
    static void parseAttribute(const nlattr *a, uint16_t maxType, RtnlAttributes &attrs, uint64_t &counter);

    struct MnlAttributeCallbackArgs
    {
        RtnlAttributes *attrs;
        uint16_t *maxType;
        uint64_t *counter;
    };

    static auto dispatchMnlAttributeCallback(const struct nlattr *a, void *self) -> int;

    [[nodiscard]] auto isEnumerating() const -> bool
    {
        return m_cacheState != CacheState::WaitingForChanges;
    }

    [[nodiscard]] auto isEnumeratingLinks() const -> bool
    {
        return m_cacheState == CacheState::EnumeratingLinks;
    }

    [[nodiscard]] auto isEnumeratingAddresses() const -> bool
    {
        return m_cacheState == CacheState::EnumeratingAddresses;
    }

    [[nodiscard]] auto isEnumeratingRoutes() const -> bool
    {
        return m_cacheState == CacheState::EnumeratingRoutes;
    }

    void notifyChanges();
    void notifyInterfacesSnapshot();
    [[nodiscard]] auto getInterfacesSnapshot() const -> Interfaces;

    std::unique_ptr<mnl_socket, int (*)(mnl_socket *)> m_mnlSocket;
    std::vector<uint8_t> m_buffer;
    bool m_running{false};
    uint32_t m_portid{};
    uint32_t m_sequenceNumber{};

    std::map<uint32_t, NetworkInterfaceStatusTracker> m_trackers;

    enum class CacheState : uint8_t
    {
        EnumeratingLinks,
        EnumeratingAddresses,
        EnumeratingRoutes,
        WaitingForChanges
    } m_cacheState{CacheState::EnumeratingLinks};

    struct Statistics
    {
        std::chrono::time_point<std::chrono::steady_clock> startTime;
        uint64_t bytesSent{};
        uint64_t bytesReceived{};
        uint64_t packetsSent{};
        uint64_t packetsReceived{};
        uint64_t msgsReceived{};
        uint64_t msgsDiscarded{};
        uint64_t seenAttributes{};
        uint64_t addressMessagesSeen{};
        uint64_t linkMessagesSeen{};
        uint64_t routeMessagesSeen{};
    } m_stats;

    RuntimeOptions m_runtimeOptions;
    InterfacesNotifier m_interfacesNotifier;
    OperationalStateNotifier m_operationalStateNotifier;
    NetworkAddressNotifier m_networkAddressNotifier;
    GatewayAddressNotifier m_gatewayAddressNotifier;
    MacAddressNotifier m_macAddressNotifier;
    BroadcastAddressNotifier m_broadcastAddressNotifier;
    EnumerationDoneNotifier m_enumerationDoneNotifier;
};
} // namespace monkas
