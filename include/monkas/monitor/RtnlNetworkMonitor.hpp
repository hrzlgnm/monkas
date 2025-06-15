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
inline std::ostream &operator<<(std::ostream &os, uint8_t c)
{
    return os << static_cast<uint16_t>(c);
}

// TODO: wrap this into a own class with validating getters
using RtnlAttributes = std::vector<const nlattr *>;

enum RuntimeFlag : uint32_t
{
    StatsForNerds = 1,
    PreferredFamilyV4 = 2,
    PreferredFamilyV6 = 4,
    DumpPackets = 8,
    NonBlocking = 16,
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

using EthernetAddressNotifier = Watchable<network::Interface, const ethernet::Address>;
using EthernetAddressWatcher = EthernetAddressNotifier::Watcher;
using EthernetAddressWatcherToken = EthernetAddressNotifier::Token;

using EnumerationDoneNotifier = Watchable<>;
using EnumerationDoneWatcher = EnumerationDoneNotifier::Watcher;
using EnumerationDoneWatcherToken = EnumerationDoneNotifier::Token;

class RtnlNetworkMonitor
{
  public:
    explicit RtnlNetworkMonitor(const RuntimeOptions &options);
    void enumerateInterfaces();
    int run();
    void stop();

    [[nodiscard]] InterfacesWatcherToken addInterfacesWatcher(const InterfacesWatcher &watcher,
                                                              bool initialSnapshot = false);
    void removeInterfacesWatcher(const InterfacesWatcherToken &token);

    [[nodiscard]] OperationalStateWatcherToken addOperationalStateWatcher(const OperationalStateWatcher &watcher,
                                                                          bool initialSnapshot = false);
    void removeOperationalStateWatcher(const OperationalStateWatcherToken &token);

    [[nodiscard]] NetworkAddressWatcherToken addNetworkAddressWatcher(const NetworkAddressWatcher &watcher,
                                                                      bool initialSnapshot = false);
    void removeNetworkAddressWatcher(const NetworkAddressWatcherToken &token);

    [[nodiscard]] GatewayAddressWatcherToken addGatewayAddressWatcher(const GatewayAddressWatcher &watcher,
                                                                      bool initialSnapshot = false);
    void removeGatewayAddressWatcher(const GatewayAddressWatcherToken &token);

    [[nodiscard]] EthernetAddressWatcherToken addEthernetAddressWatcher(const EthernetAddressWatcher &watcher,
                                                                        bool initialSnapshot = false);
    void removeEthernetAddressWatcher(const EthernetAddressWatcherToken &token);

    // enumeration done watcher is called when enumeration is done, or immediately if enumeration is already done
    // the optional return value is used to indicate that enumeration is already done
    [[nodiscard]] std::optional<EnumerationDoneWatcherToken> addEnumerationDoneWatcher(
        const EnumerationDoneWatcher &watcher);
    void removeEnumerationDoneWatcher(const EnumerationDoneWatcherToken &token);

  private:
    void receiveAndProcess();

    /* @note: only one such request can be in progress until the reply is received */
    void sendDumpRequest(uint16_t msgType);

    void parseLinkMessage(const nlmsghdr *nlhdr, const ifinfomsg *ifi);
    void parseAddressMessage(const nlmsghdr *nlhdr, const ifaddrmsg *ifa);
    void parseRouteMessage(const nlmsghdr *nlhdr, const rtmsg *rt);

    NetworkInterfaceStatusTracker &ensureNameCurrent(int ifIndex, const nlattr *nameAttribute);

    void printStatsForNerdsIfEnabled();

    int mnlMessageCallback(const nlmsghdr *n);
    static int dispatchMnMessageCallbackToSelf(const struct nlmsghdr *n, void *self);

    RtnlAttributes parseAttributes(const nlmsghdr *n, size_t offset, uint16_t maxtype);
    static void parseAttribute(const nlattr *a, uint16_t maxType, RtnlAttributes &attrs, uint64_t &counter);

    struct MnlAttributeCallbackArgs
    {
        RtnlAttributes *attrs;
        uint16_t *maxType;
        uint64_t *counter;
    };

    static int dispatchMnlAttributeCallback(const struct nlattr *a, void *self);

    inline bool isEnumerating()
    {
        return m_cacheState != CacheState::WaitingForChanges;
    }

    inline bool isEnumeratingLinks() const
    {
        return m_cacheState == CacheState::EnumeratingLinks;
    }

    inline bool isEnumeratingAddresses() const
    {
        return m_cacheState == CacheState::EnumeratingAddresses;
    }

    inline bool isEnumeratingRoutes() const
    {
        return m_cacheState == CacheState::EnumeratingRoutes;
    }

    void notifyChanges();
    void notifyInterfacesSnapshot();
    Interfaces getInterfacesSnapshot() const;

  private:
    std::unique_ptr<mnl_socket, int (*)(mnl_socket *)> m_mnlSocket;
    std::vector<uint8_t> m_buffer;
    bool m_running{false};
    uint32_t m_portid{};
    uint32_t m_sequenceNumber{};

    std::map<int, NetworkInterfaceStatusTracker> m_trackers;

    enum class CacheState
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
    EthernetAddressNotifier m_ethernetAddressNotifier;
    EnumerationDoneNotifier m_enumerationDoneNotifier;
};
} // namespace monkas
