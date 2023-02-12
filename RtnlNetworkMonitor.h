#ifndef RTNLNETWORKMONITOR_H
#define RTNLNETWORKMONITOR_H

#include "NetworkInterface.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

// TODO: sometimes an enthernet interface comes up with Unknown operstate, ip link shows the same info, what to do in
// this case.
// TODO: validate rtattrs before using them
// TODO: consider ENOBUFS errno from recv as resync point

struct nlmsghdr;
struct ifinfomsg;
struct ifaddrmsg;
struct rtmsg;
struct mnl_socket;
struct nlattr;
namespace monkas
{

// TODO: wrap this into a own class with validation and getters
using RtnlAttributes = std::vector<const nlattr *>;

class RtnlNetworkMonitor
{
  public:
    RtnlNetworkMonitor();
    int run();

  private:
    void startReceiving();

    /* @note: only one such request can be in progress until the reply is fully
     * received */
    void sendDumpRequest(uint16_t msgType);

    void parseLinkMessage(const nlmsghdr *nlhdr, const ifinfomsg *ifi);
    void parseAddressMessage(const nlmsghdr *nlhdr, const ifaddrmsg *ifa);
    void parseRouteMessage(const nlmsghdr *nlhdr, const rtmsg *rt);

    RtnlAttributes parseAttributes(const nlmsghdr *n, size_t offset, uint16_t maxtype);

    void printStatsForNerds();

    int mnlMessageCallback(const nlmsghdr *n);

    NetworkInterface &ensureNameAndIndexCurrent(int ifIndex, const RtnlAttributes &attributes);

    static void parseAttribute(const nlattr *a, uint16_t maxType, RtnlAttributes &attrs, uint64_t &counter);
    static int dipatchMnlDataCallbackToSelf(const struct nlmsghdr *n, void *self);

    struct MnlAttributeCallbackArgs
    {
        RtnlAttributes *attrs;
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
    std::map<int, NetworkInterface> m_cache;

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
        uint64_t addressMessagesSeen{};
        uint64_t linkMessagesSeen{};
        uint64_t routeMessagesSeen{};
    } m_stats;
};
} // namespace monkas
#endif
