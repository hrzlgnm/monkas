#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include <ip/Address.hpp>
#include <monitor/NetworkInterfaceStatusTracker.hpp>
#include <network/Interface.hpp>
#include <sys/types.h>
#include <util/FlagSet.hpp>

struct mnl_socket;
struct nlmsghdr;
struct ifinfomsg;
struct ifaddrmsg;
struct rtmsg;

namespace monkas::monitor
{

enum class RuntimeFlag : uint8_t
{
    StatsForNerds,
    PreferredFamilyV4,
    PreferredFamilyV6,
    IncludeNonIeee802,
    DumpPackets,
    NonBlocking,
    // NOTE: keep FlagsCount last
    FlagsCount,
};

using RuntimeFlags = util::FlagSet<RuntimeFlag>;
using Interfaces = std::set<network::Interface>;
using LinkFlags = NetworkInterfaceStatusTracker::LinkFlags;
using OperationalState = NetworkInterfaceStatusTracker::OperationalState;

struct Subscriber
{
    Subscriber() = default;
    virtual ~Subscriber() = default;
    Subscriber(const Subscriber&) = delete;
    Subscriber(Subscriber&&) = delete;
    auto operator=(const Subscriber&) -> Subscriber& = delete;
    auto operator=(Subscriber&&) -> Subscriber& = delete;

    virtual void onInterfaceAdded(const network::Interface& /*unused*/) {}

    virtual void onInterfaceRemoved(const network::Interface& /*unused*/) {}

    virtual void onInterfaceNameChanged(const network::Interface& /*unused*/) {}

    virtual void onLinkFlagsChanged(const network::Interface& /*unused*/, const LinkFlags& /*unused*/) {}

    virtual void onOperationalStateChanged(const network::Interface& /*unused*/, OperationalState /*unused*/) {}

    virtual void onNetworkAddressesChanged(const network::Interface& /*unused*/, const Addresses& /*unused*/) {}

    virtual void onGatewayAddressChanged(const network::Interface& /*unused*/,
                                         const std::optional<ip::Address>& /*unused*/)
    {
    }

    virtual void onMacAddressChanged(const network::Interface& /*unused*/, const ethernet::Address& /*unused*/) {}

    virtual void onBroadcastAddressChanged(const network::Interface& /*unused*/, const ethernet::Address& /*unused*/) {}
};

using SubscriberPtr = std::shared_ptr<Subscriber>;

class NetworkMonitor
{
  public:
    explicit NetworkMonitor(const RuntimeFlags& options);
    auto enumerateInterfaces() -> Interfaces;
    void subscribe(const Interfaces& interfaces, const SubscriberPtr& subscriber);
    void updateSubscription(const Interfaces& interfaces, const SubscriberPtr& subscriber);
    void unsubscribe(const SubscriberPtr& subscriber);
    void run();
    void stop();

  private:
    void receiveAndProcess();
    auto interfacesFromCache() -> Interfaces;
    void updateStats(ssize_t receiveResult);
    void dumpPacket(ssize_t receiveResult) const;
    auto handleCallbackResult(int callbackResult) -> bool;

    /* @note: only one such request can be in progress until the reply is received */
    void sendDumpRequest(uint16_t msgType);
    void retryLastDumpRequest();

    auto ensureNameCurrent(uint32_t ifIndex, const std::optional<std::string>& name) -> NetworkInterfaceStatusTracker&;

    void parseLinkMessage(const nlmsghdr* nlhdr, const ifinfomsg* ifi);
    void parseAddressMessage(const nlmsghdr* nlhdr, const ifaddrmsg* ifa);
    void parseRouteMessage(const nlmsghdr* nlhdr, const rtmsg* rtm);

    void printStatsForNerdsIfEnabled();

    auto mnlMessageCallback(const nlmsghdr* n) -> int;
    static auto dispatchMnMessageCallbackToSelf(const nlmsghdr* n, void* self) -> int;

    [[nodiscard]] auto isEnumerating() const -> bool { return m_cacheState != CacheState::WaitingForChanges; }

    [[nodiscard]] auto isEnumeratingLinks() const -> bool { return m_cacheState == CacheState::EnumeratingLinks; }

    [[nodiscard]] auto isEnumeratingAddresses() const -> bool
    {
        return m_cacheState == CacheState::EnumeratingAddresses;
    }

    [[nodiscard]] auto isEnumeratingRoutes() const -> bool { return m_cacheState == CacheState::EnumeratingRoutes; }

    void notifyChanges();
    void notifyChanges(Subscriber* subscriber, const Interfaces& interfaces);
    void notifyInterfaceAdded(const network::Interface& interface);
    void notifyInterfaceRemoved(const network::Interface& interface);

    std::unique_ptr<mnl_socket, int (*)(mnl_socket*)> m_mnlSocket;
    std::vector<uint8_t> m_receiveBuffer;
    std::vector<uint8_t> m_sendBuffer;
    bool m_running {false};
    uint32_t m_portid {};
    uint32_t m_sequenceNumber {};

    std::map<uint32_t, NetworkInterfaceStatusTracker> m_trackers;

    enum class CacheState : uint8_t
    {
        EnumeratingLinks,
        EnumeratingAddresses,
        EnumeratingRoutes,
        WaitingForChanges
    } m_cacheState {CacheState::EnumeratingLinks};

    struct Statistics
    {
        std::chrono::time_point<std::chrono::steady_clock> startTime;
        uint64_t bytesSent {};
        uint64_t bytesReceived {};
        uint64_t packetsSent {};
        uint64_t packetsReceived {};
        uint64_t msgsReceived {};
        uint64_t msgsDiscarded {};
        uint64_t seenAttributes {};
        uint64_t unknownAttributes {};
        uint64_t addressMessagesSeen {};
        uint64_t linkMessagesSeen {};
        uint64_t routeMessagesSeen {};
    } m_stats;

    RuntimeFlags m_runtimeOptions;
    std::unordered_map<SubscriberPtr, Interfaces> m_subscribers;
};
}  // namespace monkas::monitor
