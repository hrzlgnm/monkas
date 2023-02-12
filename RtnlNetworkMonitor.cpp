#include "RtnlNetworkMonitor.h"
#include "ip/Address.h"

#include <glog/logging.h>
#include <libmnl/libmnl.h>
#include <linux/rtnetlink.h>
#include <memory.h>
#include <net/if_arp.h>

#include <iomanip>
#include <iostream>
#include <sstream>

namespace
{
unsigned toNetlinkMulticastFlag(rtnetlink_groups group)
{
    return (1 << (group - 1));
}
} // namespace

namespace monkas
{

RtnlNetworkMonitor::RtnlNetworkMonitor()
    : m_mnlSocket{mnl_socket_open(NETLINK_ROUTE), mnl_socket_close}, m_portid{mnl_socket_get_portid(m_mnlSocket.get())}
{
    m_stats.startTime = std::chrono::steady_clock::now();
    m_buffer.resize(MNL_SOCKET_BUFFER_SIZE);
    unsigned groups = toNetlinkMulticastFlag(RTNLGRP_LINK);
    groups |= toNetlinkMulticastFlag(RTNLGRP_NOTIFY);
    groups |= toNetlinkMulticastFlag(RTNLGRP_IPV4_IFADDR);
    groups |= toNetlinkMulticastFlag(RTNLGRP_IPV6_IFADDR);
    groups |= toNetlinkMulticastFlag(RTNLGRP_IPV4_ROUTE);
    groups |= toNetlinkMulticastFlag(RTNLGRP_IPV6_ROUTE);
    VLOG(1) << "Joining RTnetlink multicast groups " << std::hex << std::setfill('0') << std::setw(8) << groups;
    if (mnl_socket_bind(m_mnlSocket.get(), groups, MNL_SOCKET_AUTOPID) < 0)
    {
        PLOG(FATAL) << "mnl_socket_bind";
    }
}

int RtnlNetworkMonitor::run()
{
    LOG(INFO) << "requesting RTM_GETLINK";
    // start with link information, to setup the cache to only contain ethernet devices
    sendDumpRequest(RTM_GETLINK);
    startReceiving();
    return 0;
}

void RtnlNetworkMonitor::startReceiving()
{
    auto receiveResult = mnl_socket_recvfrom(m_mnlSocket.get(), &m_buffer[0], m_buffer.size());
    while (receiveResult > 0)
    {
        m_stats.packetsReceived++;
        m_stats.bytesReceived += receiveResult;
        if (VLOG_IS_ON(5))
        {
            fflush(stderr);
            fflush(stdout);
            mnl_nlmsg_fprintf(stdout, &m_buffer[0], receiveResult, 0);
        }
        auto seqNo = isEnumerating() ? m_sequenceNumber : 0;
        auto callbackResult = mnl_cb_run(&m_buffer[0], receiveResult, seqNo, m_portid,
                                         &RtnlNetworkMonitor::dipatchMnlDataCallbackToSelf, this);
        if (callbackResult == MNL_CB_ERROR)
        {
            PLOG(WARNING) << "bork";
            break;
        }
        if (callbackResult == MNL_CB_STOP)
        {
            if (isEnumeratingLinks())
            {
                m_cacheState = CacheState::EnumeratingAddresses;
                LOG(INFO) << "requesting RTM_GETADDR";
                sendDumpRequest(RTM_GETADDR);
            }
            else if (isEnumeratingAddresses())
            {
                m_cacheState = CacheState::EnumeratingRoutes;
                LOG(INFO) << "requesting RTM_GETROUTE";
                sendDumpRequest(RTM_GETROUTE);
            }
            else if (isEnumeratingRoutes())
            {
                m_cacheState = CacheState::WaitForChangeNotifications;
                LOG(INFO) << "Done with enumeration of initial information";
                LOG(INFO) << "tracking changes for: " << m_cache.size() << " interfaces";
                printStatsForNerds();
            }
            else
            {
                LOG(WARNING) << "Unexpected MNL_CB_STOP value";
            }
        }
        receiveResult = mnl_socket_recvfrom(m_mnlSocket.get(), &m_buffer[0], m_buffer.size());
    }
}

void RtnlNetworkMonitor::sendDumpRequest(uint16_t msgType)
{
    nlmsghdr *nlh = mnl_nlmsg_put_header(&m_buffer[0]);
    nlh->nlmsg_type = msgType;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nlh->nlmsg_seq = ++m_sequenceNumber;
    ifinfomsg *ifi = (ifinfomsg *)mnl_nlmsg_put_extra_header(nlh, sizeof(struct ifinfomsg));
    ifi->ifi_family = AF_UNSPEC;
    mnl_attr_put_u32(nlh, IFLA_EXT_MASK, 1 << 3); // RTEXT_FILTER_SKIP_STATS
    auto ret = mnl_socket_sendto(m_mnlSocket.get(), nlh, nlh->nlmsg_len);
    if (ret < 0)
    {
        PLOG(FATAL) << "mnl_socket_sendto";
    }
    m_stats.packetsSent++;
    m_stats.bytesSent += ret;
}

int RtnlNetworkMonitor::mnlMessageCallback(const nlmsghdr *n)
{
    m_stats.msgsReceived++;
    const auto t = n->nlmsg_type;
    switch (t)
    {
    case RTM_NEWLINK:
        // fallthrough
    case RTM_DELLINK: {
        const ifinfomsg *ifi = (const ifinfomsg *)mnl_nlmsg_get_payload(n);
        parseLinkMessage(n, ifi);
    }
    break;
    case RTM_NEWADDR:
        // fallthrough
    case RTM_DELADDR: {
        const ifaddrmsg *ifa = (const ifaddrmsg *)mnl_nlmsg_get_payload(n);
        parseAddressMessage(n, ifa);
    }
    break;
    case RTM_NEWROUTE:
        // fallthrough
    case RTM_DELROUTE: {
        const rtmsg *rt = (const rtmsg *)mnl_nlmsg_get_payload(n);
        parseRouteMessage(n, rt);
    }
    break;
    default:
        LOG(WARNING) << "ignoring unexpected message type: " << t;
        break;
    }
    return MNL_CB_OK;
}

void RtnlNetworkMonitor::parseAttribute(const nlattr *a, uint16_t maxType, RtnlAttributes &attrs, uint64_t &counter)
{
    const auto type = mnl_attr_get_type(a);
    if (mnl_attr_type_valid(a, maxType) > 0)
    {
        counter++;
        attrs.at(type) = a;
    }
    else
    {
        PLOG(WARNING) << "ignoring unexpected nlattr type " << type;
    }
}

NetworkInterface &RtnlNetworkMonitor::ensureNameAndIndexCurrent(int ifIndex, const RtnlAttributes &attributes)
{
    auto &cacheEntry = m_cache[ifIndex];
    cacheEntry.setIndex(ifIndex);
    if (cacheEntry.hasName())
    {
        // sometimes interfaces are renamed, account for that
        if (attributes[IFLA_IFNAME])
        {
            auto nameFromAttributes{mnl_attr_get_str(attributes[IFLA_IFNAME])};
            cacheEntry.setName(nameFromAttributes);
        }
        return cacheEntry;
    }
    // it's safe to use IFLA_IFNAME here as IFA_LABEL is the same id
    // we should be receiving link messages as first anyway
    if (attributes[IFLA_IFNAME])
    {
        VLOG(3) << "using name from attributes";
        m_stats.resolveIfNameByAttributes++;
        cacheEntry.setName(mnl_attr_get_str(attributes[IFLA_IFNAME]));
    }
    else
    {
        LOG(WARNING) << "failed to determine interface name from attributes" << ifIndex << "\n";
    }
    return cacheEntry;
}

void RtnlNetworkMonitor::parseLinkMessage(const nlmsghdr *nlhdr, const ifinfomsg *ifi)
{
    m_stats.linkMessagesSeen++;
    // TODO ifi_type filters
    if (ifi->ifi_type != ARPHRD_ETHER)
    {
        VLOG(4) << "ignoring non ethernet network interface with index: " << ifi->ifi_index;
        m_stats.msgsDiscarded++;
        return;
    }
    if (nlhdr->nlmsg_type == RTM_DELLINK)
    {
        VLOG(2) << "removing interface with index" << ifi->ifi_index;
        m_cache.erase(ifi->ifi_index);
        return;
    }
    auto attributes = parseAttributes(nlhdr, sizeof(*ifi), IFLA_MAX);

    auto &cacheEntry = ensureNameAndIndexCurrent(ifi->ifi_index, attributes);
    if (attributes[IFLA_OPERSTATE])
    {
        auto operstate{mnl_attr_get_u16(attributes[IFLA_OPERSTATE])};
        cacheEntry.setOperationalState(static_cast<OperationalState>(operstate));
    }
    if (attributes[IFLA_ADDRESS])
    {
        const uint8_t *addr = (const uint8_t *)mnl_attr_get_payload(attributes[IFLA_ADDRESS]);
        const auto len = mnl_attr_get_payload_len(attributes[IFLA_ADDRESS]);
        auto ethernetAddress = ethernet::Address::fromBytes(addr, len);
        cacheEntry.setEthernetAddress(ethernetAddress);
    }
    if (attributes[IFLA_BROADCAST])
    {
        const uint8_t *addr = (const uint8_t *)mnl_attr_get_payload(attributes[IFLA_BROADCAST]);
        const auto len = mnl_attr_get_payload_len(attributes[IFLA_BROADCAST]);
        auto ethernetAddress = ethernet::Address::fromBytes(addr, len);
        cacheEntry.setBroadcastAddress(ethernetAddress);
    }
    printStatsForNerds();
}

void RtnlNetworkMonitor::parseAddressMessage(const nlmsghdr *nlhdr, const ifaddrmsg *ifa)
{
    m_stats.addressMessagesSeen++;
    if (m_cache.find(ifa->ifa_index) == m_cache.cend())
    {
        VLOG(4) << "unknown network interface index: " << ifa->ifa_index;
        m_stats.msgsDiscarded++;
        return;
    }
    auto attributes = parseAttributes(nlhdr, sizeof(*ifa), IFA_MAX);

    uint32_t flags = ifa->ifa_flags; // will be overwritten if IFA_FLAGS is present
    ip::Address address;
    ip::Address broadcast;

    auto &cacheEntry = ensureNameAndIndexCurrent(ifa->ifa_index, attributes);
    if (attributes[IFA_FLAGS])
    {
        flags = {mnl_attr_get_u32(attributes[IFA_FLAGS])};
    }

    if (attributes[IFA_BROADCAST])
    {
        auto addr_len = mnl_attr_get_payload_len(attributes[IFA_BROADCAST]);
        if (addr_len == ip::IPV4_ADDR_LEN)
        {
            const uint8_t *addr = (const uint8_t *)mnl_attr_get_payload(attributes[IFA_BROADCAST]);
            broadcast = ip::Address::fromBytes(addr, addr_len);
            VLOG(1) << "broadcast " << broadcast;
        }
    }
    if (attributes[IFA_LOCAL])
    {
        auto addr_len = mnl_attr_get_payload_len(attributes[IFA_LOCAL]);
        if (addr_len == ip::IPV4_ADDR_LEN)
        {
            const uint8_t *addr = (const uint8_t *)mnl_attr_get_payload(attributes[IFA_LOCAL]);
            address = ip::Address::fromBytes(addr, addr_len);
        }
    }
    if (attributes[IFA_ADDRESS])
    {
        const uint8_t *addr = (const uint8_t *)mnl_attr_get_payload(attributes[IFA_ADDRESS]);
        auto addr_len = mnl_attr_get_payload_len(attributes[IFA_ADDRESS]);
        if (addr_len == ip::IPV6_ADDR_LEN)
        {
            address = ip::Address::fromBytes(addr, addr_len);
        }
    }
    const network::NetworkAddress networkAddress{
        address.adressFamily(), address, broadcast, ifa->ifa_prefixlen, network::fromRtnlScope(ifa->ifa_scope), flags};
    if (nlhdr->nlmsg_type == RTM_NEWADDR)
    {
        cacheEntry.addNetworkAddress(networkAddress);
    }
    else if (nlhdr->nlmsg_type == RTM_DELADDR)
    {
        cacheEntry.removeNetworkAddress(networkAddress);
    }
    printStatsForNerds();
}

void RtnlNetworkMonitor::parseRouteMessage(const nlmsghdr *nlhdr, const rtmsg *rtm)
{
    m_stats.routeMessagesSeen++;
    if (rtm->rtm_family != AF_INET)
    {
        m_stats.msgsDiscarded++;
        VLOG(4) << "ignoring address family: " << rtm->rtm_family;
        return;
    }
    auto attributes = parseAttributes(nlhdr, sizeof(*rtm), RTA_MAX);
    if (nlhdr->nlmsg_type == RTM_DELROUTE)
    {
        // remove ipv4 routing default gateway when a linkdown was detected for the interface
        if (rtm->rtm_flags & RTNH_F_LINKDOWN && attributes[RTA_OIF])
        {
            auto outIfIndex = mnl_attr_get_u32(attributes[RTA_OIF]);
            auto itr = m_cache.find(outIfIndex);
            if (itr != m_cache.end() && itr->second.gatewayAddress())
            {
                VLOG(1) << "Removing gateway " << itr->second.gatewayAddress() << " due to linkdown";
                itr->second.setGatewayAddress(ip::Address());
            }
        }
        return;
    }
    if (attributes[RTA_GATEWAY] && attributes[RTA_OIF])
    {
        auto outif = mnl_attr_get_u32(attributes[RTA_OIF]);
        auto itr = m_cache.find(outif);
        if (itr != m_cache.end())
        {
            const uint8_t *addr = (const uint8_t *)mnl_attr_get_payload(attributes[RTA_GATEWAY]);
            auto len = mnl_attr_get_payload_len(attributes[RTA_GATEWAY]);
            auto gatewayAddress = ip::Address::fromBytes(addr, len);
            itr->second.setGatewayAddress(gatewayAddress);
        }
    }
}

RtnlAttributes RtnlNetworkMonitor::parseAttributes(const nlmsghdr *n, size_t offset, uint16_t maxType)
{
    RtnlAttributes::size_type typesToAllocate = maxType + 1;
    RtnlAttributes attributes{typesToAllocate};
    MnlAttributeCallbackArgs arg{&attributes, &maxType, &m_stats.seenAttributes};
    mnl_attr_parse(n, offset, &RtnlNetworkMonitor::dispatchMnlAttributeCallback, &arg);
    return attributes;
}

// TODO: enable/disable via command line options instead of VLOG
void RtnlNetworkMonitor::printStatsForNerds()
{
    if (m_cacheState != CacheState::WaitForChangeNotifications)
    {
        return;
    }
    VLOG(1) << "--------------- Stats for nerds -----------------";
    VLOG(1) << "uptime              "
            << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                                     m_stats.startTime)
                   .count()
            << "ms";
    VLOG(1) << "sent                " << m_stats.bytesSent << " bytes in " << m_stats.packetsSent << " packets";
    VLOG(1) << "received            " << m_stats.bytesReceived << " bytes in " << m_stats.packetsReceived << " pakets";
    VLOG(1) << "received            " << m_stats.msgsReceived << " netlink messages";
    VLOG(1) << "discarded           " << m_stats.msgsDiscarded << " netlink messages";
    VLOG(1) << "* seen";
    VLOG(1) << "                    " << m_stats.seenAttributes << " attribute entries";
    VLOG(1) << "                    " << m_stats.linkMessagesSeen << " link messages";
    VLOG(1) << "                    " << m_stats.addressMessagesSeen << " address messages";
    VLOG(1) << "                    " << m_stats.routeMessagesSeen << " route messages";
    VLOG(1) << "* resolved ifname";
    VLOG(1) << "                    " << m_stats.resolveIfNameByAttributes << " via attributes";

    VLOG(1) << "----------- Interface details in cache ----------";
    for (const auto &c : m_cache)
    {
        VLOG(1) << c.second;
    }
    VLOG(1) << "-------------------------------------------------";
}

int RtnlNetworkMonitor::dipatchMnlDataCallbackToSelf(const nlmsghdr *n, void *self)
{
    return reinterpret_cast<RtnlNetworkMonitor *>(self)->mnlMessageCallback(n);
}

int RtnlNetworkMonitor::dispatchMnlAttributeCallback(const nlattr *a, void *ud)
{
    auto args = reinterpret_cast<MnlAttributeCallbackArgs *>(ud);
    parseAttribute(a, *args->maxType, *args->attrs, *args->counter);
    return MNL_CB_OK;
}

} // namespace monkas
