#ifndef RTNETLINKETHERNETLINKANDIPADDRESSMONITOR_H
#define RTNETLINKETHERNETLINKANDIPADDRESSMONITOR_H

#include "NetworkInterfaceDescriptor.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

// TODO: sometimes an enthernet interface comes up with Unknown operstate, ip link shows the same info, what to do in
// this case.
// TODO: parse Route messages to determine gateway addresses
// TODO: validate rtattrs before using them
// TODO: consider ENOBUFS errno from recv as resync point

struct nlmsghdr;
struct ifinfomsg;
struct ifaddrmsg;
struct rtmsg;
struct mnl_socket;
struct nlattr;

using RtAttributes = std::vector<const nlattr *>;

class RtNetlinkEthernetLinkAndIpAddressMonitor
{
  public:
    RtNetlinkEthernetLinkAndIpAddressMonitor();
    int run();

  private:
    void startReceiving();

    /* @note: only one such request can be in progress until the reply is fully
     * received */
    void sendDumpRequest(uint16_t msgType);

    // TODO: consider passing nlmsghdr instead of the bool flag
    void parseLinkMessage(const nlmsghdr *nlhdr, const ifinfomsg *ifi);
    void parseAddressMessage(const nlmsghdr *nlhdr, const ifaddrmsg *ifa);
    void parseRouteMessage(const nlmsghdr *nlhdr, const rtmsg *rt);

    RtAttributes parseAttributes(const nlmsghdr *n, size_t offset, uint16_t maxtype);

    void printStatsForNerds();

    int mnlMessageCallback(const nlmsghdr *n);

    NetworkInterfaceDescriptor &ensureNameAndIndexCurrent(int ifIndex, const RtAttributes &attributes);

    static void parseAttribute(const nlattr *a, uint16_t maxType, RtAttributes &attrs, uint64_t &counter);
    static int dipatchMnlDataCallbackToSelf(const struct nlmsghdr *n, void *self);

    struct MnlAttributeCallbackArgs
    {
        RtAttributes *attrs;
        uint16_t *maxType;
        uint64_t *counter;
    };

    static int dispatchMnlAttributeCallback(const struct nlattr *a, void *self);

    inline bool isEnumerating()
    {
        return m_cacheState != CacheState::WaitForChangeNotifications;
    }

    inline bool isEnumeratingLinks() const
    {
        return m_cacheState == CacheState::EnumeringLinks;
    }
    inline bool isEnumeratingAddresses() const
    {
        return m_cacheState == CacheState::EnumeratingAddresses;
    }
    inline bool isEnumeratingRoutes() const
    {
        return m_cacheState == CacheState::EnumeratingRoutes;
    }

  private:
    std::vector<char> m_buffer;
    std::unique_ptr<mnl_socket, int (*)(mnl_socket *)> m_mnlSocket;
    std::map<int, NetworkInterfaceDescriptor> m_cache;

    // TODO: fill -> enumerate
    enum class CacheState
    {
        EnumeringLinks,
        EnumeratingAddresses,
        EnumeratingRoutes,
        WaitForChangeNotifications
    } m_cacheState{CacheState::EnumeringLinks};

    unsigned m_portid{};
    unsigned m_sequenceNumber{};

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
        uint64_t resolveIfNameByAttributes{};
        uint64_t resolveIfNameByIfIndexToName{};
        uint64_t addressUpdatesProcessed{};
        uint64_t linkUpdatesProcessed{};
    } m_stats;
};

#endif
