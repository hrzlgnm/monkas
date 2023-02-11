#ifndef RTNETLINKETHERNETLINKANDIPADDRESSMONITOR_H
#define RTNETLINKETHERNETLINKANDIPADDRESSMONITOR_H

#include "NetworkInterfaceDescriptor.h"
#include <libmnl/libmnl.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

// TODO: sometimes an enthernet interface comes up with Unknown operstate, ip link shows the same info, what to do in
// this case.
class RtNetlinkEthernetLinkAndIpAddressMonitor
{
  public:
    RtNetlinkEthernetLinkAndIpAddressMonitor();
    int run();

  private:
    void startReceiving();

    /* @note: only one such request can be in progress until the reply is fully
     * received */
    void request(uint16_t msgType);

    void handleLinkMessage(const struct ifinfomsg *ifi, bool isNew);
    void handleAddrMessage(const struct ifaddrmsg *ifa, bool isNew);
    void parseAttributes(const struct nlmsghdr *n, size_t offset, uint16_t maxtype);
    NetworkInterfaceDescriptor &ensureNameAndIndexCurrent(int interface_index);

    void dumpState();

    int mnlMessageCallback(const struct nlmsghdr *n);
    int mnlAttributeCallback(const nlattr *a);

    static int dipatchMnlDataCallbackToSelf(const struct nlmsghdr *n, void *self);
    static int dispatchMnlAttributeCallbackToSelf(const struct nlattr *a, void *self);

  private:
    std::vector<const nlattr *> m_lastSeenAttributes;
    std::vector<char> m_buffer;
    std::unique_ptr<mnl_socket, int (*)(mnl_socket *)> m_mnlSocket;
    std::map<int, NetworkInterfaceDescriptor> m_cache;

    enum class CacheState
    {
        FillLinkInfo,
        FillAddressInfo,
        WaitForChanges
    } m_cacheState{CacheState::FillLinkInfo};

    unsigned m_portid{};
    unsigned m_sequenceCounter{1};
    uint16_t m_maxAttributeType{};

    struct Statistics
    {
        uint64_t bytesSent{};
        uint64_t bytesReceived{};
        uint64_t packetsSent{};
        uint64_t packetsReceived{};
        uint64_t msgsReceived{};
        uint64_t parsedAttributes{};
    } m_stats;
};

#endif
