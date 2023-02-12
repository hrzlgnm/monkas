#include "RtNetlinkEthernetLinkAndIpAddressMonitor.h"

#include <arpa/inet.h>
#include <glog/logging.h>
#include <libmnl/libmnl.h>
#include <linux/if_link.h>
#include <linux/rtnetlink.h>
#include <memory.h>
#include <net/if.h>
#include <net/if_arp.h>

#include <assert.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

namespace
{
unsigned toNetlinkMulticastFlag(rtnetlink_groups group)
{
    return (1 << (group - 1));
}
} // namespace

RtNetlinkEthernetLinkAndIpAddressMonitor::RtNetlinkEthernetLinkAndIpAddressMonitor()
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

int RtNetlinkEthernetLinkAndIpAddressMonitor::run()
{
    LOG(INFO) << "requesting RTM_GETLINK";
    // start with link information, to setup the cache to only contain ethernet devices
    sendDumpRequest(RTM_GETLINK);
    startReceiving();
    return 0;
}

void RtNetlinkEthernetLinkAndIpAddressMonitor::startReceiving()
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
                                         &RtNetlinkEthernetLinkAndIpAddressMonitor::dipatchMnlDataCallbackToSelf, this);
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
                LOG(INFO) << "cache is filled with all requested information";
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

void RtNetlinkEthernetLinkAndIpAddressMonitor::sendDumpRequest(uint16_t msgType)
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

int RtNetlinkEthernetLinkAndIpAddressMonitor::mnlMessageCallback(const nlmsghdr *n)
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

void RtNetlinkEthernetLinkAndIpAddressMonitor::parseAttribute(const nlattr *a, uint16_t maxType, RtAttributes &attrs,
                                                              uint64_t &counter)
{
    if (mnl_attr_type_valid(a, maxType) >= 0)
    {
        counter++;
        auto type = mnl_attr_get_type(a);
        attrs.at(type) = a;
    }
}

NetworkInterfaceDescriptor &RtNetlinkEthernetLinkAndIpAddressMonitor::ensureNameAndIndexCurrent(
    int ifIndex, const RtAttributes &attributes)
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
        VLOG(2) << "using name from already parsed attributes";
        m_stats.resolveIfNameByAttributes++;
        cacheEntry.setName(mnl_attr_get_str(attributes[IFLA_IFNAME]));
    }
    else
    {
        std::vector<char> namebuf(IF_NAMESIZE, '\0');
        auto name = if_indextoname(ifIndex, &namebuf[0]);
        if (name)
        {
            VLOG(2) << "resolved name via if_indextoname";
            m_stats.resolveIfNameByIfIndexToName++;
            cacheEntry.setName(name);
        }
        else
        {
            LOG(WARNING) << "failed to determine interface name from index " << ifIndex << "\n";
        }
    }
    return cacheEntry;
}

void RtNetlinkEthernetLinkAndIpAddressMonitor::parseLinkMessage(const nlmsghdr *nlhdr, const ifinfomsg *ifi)
{
    m_stats.linkUpdatesProcessed++;
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
    if (attributes[IFLA_CARRIER])
    {
        VLOG(3) << "carrier " << static_cast<int>(mnl_attr_get_u8(attributes[IFLA_CARRIER]));
    }
    if (attributes[IFLA_IFALIAS])
    {
        VLOG(3) << "ifalias " << mnl_attr_get_str(attributes[IFLA_IFALIAS]);
    }
    if (attributes[IFLA_OPERSTATE])
    {
        auto operstate{mnl_attr_get_u16(attributes[IFLA_OPERSTATE])};
        cacheEntry.setOperationalState(static_cast<OperationalState>(operstate));
    }
    if (attributes[IFLA_ADDRESS])
    {
        std::ostringstream strm;
        const uint8_t *hwaddr = (const uint8_t *)mnl_attr_get_payload(attributes[IFLA_ADDRESS]);
        const auto len = mnl_attr_get_payload_len(attributes[IFLA_ADDRESS]);
        for (int i = 0; i < len; i++)
        {
            strm << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hwaddr[i]);
            if (i + 1 != len)
            {
                strm << ':';
            }
        }
        cacheEntry.setHardwareAddress(strm.str());
    }
    printStatsForNerds();
}

void RtNetlinkEthernetLinkAndIpAddressMonitor::parseAddressMessage(const nlmsghdr *nlhdr, const ifaddrmsg *ifa)
{
    m_stats.addressUpdatesProcessed++;
    if (m_cache.find(ifa->ifa_index) == m_cache.cend())
    {
        VLOG(4) << "unknown network interface index: " << ifa->ifa_index;
        m_stats.msgsDiscarded++;
        return;
    }
    auto attributes = parseAttributes(nlhdr, sizeof(*ifa), IFA_MAX);
    auto &cacheEntry = ensureNameAndIndexCurrent(ifa->ifa_index, attributes);
    // TODO: do we want to track flags? Perhaps for link state determination if operstatus is not given.
    if (attributes[IFA_FLAGS])
    {
        auto flags{mnl_attr_get_u32(attributes[IFA_FLAGS])};
        VLOG(3) << "flags " << std::setw(8) << std::setfill('0') << std::hex << flags;
    }
    if (attributes[IFA_ADDRESS])
    {
        void *addr = mnl_attr_get_payload(attributes[IFA_ADDRESS]);
        char out[INET6_ADDRSTRLEN];
        if (inet_ntop(ifa->ifa_family, addr, out, sizeof(out)))
        {
            std::stringstream strm;
            if (ifa->ifa_family == AF_INET6)
            {
                strm << "ip6=";
            }
            else if (ifa->ifa_family == AF_INET)
            {
                strm << "ip4=";
            }
            strm << out << "/" << static_cast<int>(ifa->ifa_prefixlen);
            // TODO: Factor out to own toString function
            switch (ifa->ifa_scope)
            {
            case RT_SCOPE_UNIVERSE:
                strm << ":global";
                break;
            case RT_SCOPE_SITE:
                strm << ":site";
                break;
            case RT_SCOPE_LINK:
                strm << ":link";
                break;
            case RT_SCOPE_HOST:
                strm << ":host";
                break;
            case RT_SCOPE_NOWHERE:
                strm << ":nowhere";
                break;
            default:
                strm << ":" << ifa->ifa_scope;
                break;
            }
            if (nlhdr->nlmsg_type == RTM_NEWADDR)
            {
                cacheEntry.addAddress(strm.str());
            }
            else if (nlhdr->nlmsg_type == RTM_DELADDR)
            {
                cacheEntry.delAddress(strm.str());
            }
        }
    }
    printStatsForNerds();
}

void RtNetlinkEthernetLinkAndIpAddressMonitor::parseRouteMessage(const nlmsghdr *nlhdr, const rtmsg *rtm)
{
    auto attributes = parseAttributes(nlhdr, sizeof(*rtm), RTA_MAX);
    if (attributes[RTA_GATEWAY])
    {
        void *addr = mnl_attr_get_payload(attributes[RTA_GATEWAY]);
        char out[INET6_ADDRSTRLEN];
        if (inet_ntop(rtm->rtm_family, addr, out, sizeof(out)))
        {
            VLOG(1) << "Interface index " << mnl_attr_get_u32(attributes[RTA_OIF]) << " Gateway address: " << out;
        }
    }
}

RtAttributes RtNetlinkEthernetLinkAndIpAddressMonitor::parseAttributes(const nlmsghdr *n, size_t offset,
                                                                       uint16_t maxType)
{
    RtAttributes::size_type typesToAlloc = maxType + 1;
    RtAttributes attributes{typesToAlloc};
    MnlAttributeCallbackArgs arg{&attributes, &maxType, &m_stats.seenAttributes};
    mnl_attr_parse(n, offset, &RtNetlinkEthernetLinkAndIpAddressMonitor::dispatchMnlAttributeCallback, &arg);
    return attributes;
}

void RtNetlinkEthernetLinkAndIpAddressMonitor::printStatsForNerds()
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
    VLOG(1) << "seen                " << m_stats.seenAttributes << " attributes";
    VLOG(1) << "                    " << m_stats.linkUpdatesProcessed << " link updates";
    VLOG(1) << "                    " << m_stats.addressUpdatesProcessed << " address updates";
    VLOG(1) << "resolved ifname";
    VLOG(1) << " via attributes     " << m_stats.resolveIfNameByAttributes;
    VLOG(1) << " via if_indextoname " << m_stats.resolveIfNameByIfIndexToName;

    VLOG(1) << "----------- Interface details in cache ----------";
    for (const auto &c : m_cache)
    {
        VLOG(1) << c.second;
    }
    VLOG(1) << "-------------------------------------------------";
}

int RtNetlinkEthernetLinkAndIpAddressMonitor::dipatchMnlDataCallbackToSelf(const nlmsghdr *n, void *self)
{
    return reinterpret_cast<RtNetlinkEthernetLinkAndIpAddressMonitor *>(self)->mnlMessageCallback(n);
}

int RtNetlinkEthernetLinkAndIpAddressMonitor::dispatchMnlAttributeCallback(const nlattr *a, void *ud)
{
    auto args = reinterpret_cast<MnlAttributeCallbackArgs *>(ud);
    parseAttribute(a, *args->maxType, *args->attrs, *args->counter);
    return MNL_CB_OK;
}
