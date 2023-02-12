#include "RtNetlinkEthernetLinkAndIpAddressMonitor.h"

#include <arpa/inet.h>
#include <glog/logging.h>
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
    m_lastSeenAttributes.resize(IFLA_MAX + 1);
    unsigned groups = toNetlinkMulticastFlag(RTNLGRP_LINK);
    groups |= toNetlinkMulticastFlag(RTNLGRP_IPV4_IFADDR);
    groups |= toNetlinkMulticastFlag(RTNLGRP_IPV6_IFADDR);
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
    request(RTM_GETLINK);
    startReceiving();
    return 0;
}

void RtNetlinkEthernetLinkAndIpAddressMonitor::startReceiving()
{
    auto ret = mnl_socket_recvfrom(m_mnlSocket.get(), &m_buffer[0], m_buffer.size());
    while (ret > 0)
    {
        m_stats.packetsReceived++;
        m_stats.bytesReceived += ret;
        if (VLOG_IS_ON(5))
        {
            fflush(stderr);
            fflush(stdout);
            mnl_nlmsg_fprintf(stdout, &m_buffer[0], ret, 0);
        }
        ret = mnl_cb_run(&m_buffer[0], ret, m_sequenceCounter, m_portid,
                         &RtNetlinkEthernetLinkAndIpAddressMonitor::dipatchMnlDataCallbackToSelf, this);
        if (ret < MNL_CB_STOP)
        {
            PLOG(WARNING) << "bork";
            break;
        }
        if (ret == MNL_CB_STOP)
        {
            if (m_cacheState == CacheState::FillLinkInfo)
            {
                m_cacheState = CacheState::FillAddressInfo;
                LOG(INFO) << "requesting RTM_GETADDR";
                request(RTM_GETADDR);
            }
            else if (m_cacheState == CacheState::FillAddressInfo)
            {
                m_cacheState = CacheState::WaitForChanges;
                m_sequenceCounter = 0; // stop sequence checks
                LOG(INFO) << "cache is filled with all requested information";
                LOG(INFO) << "tracking changes for: " << m_cache.size() << " interfaces";
                printStatsForNerds();
            }
        }
        ret = mnl_socket_recvfrom(m_mnlSocket.get(), &m_buffer[0], m_buffer.size());
    }
}

void RtNetlinkEthernetLinkAndIpAddressMonitor::request(uint16_t msgType)
{
    ++m_sequenceCounter;
    nlmsghdr *nlh = mnl_nlmsg_put_header(&m_buffer[0]);
    nlh->nlmsg_type = msgType;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nlh->nlmsg_seq = m_sequenceCounter;
    rtgenmsg *rt = (rtgenmsg *)mnl_nlmsg_put_extra_header(nlh, sizeof(struct rtgenmsg));
    rt->rtgen_family = AF_PACKET;
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
        ifinfomsg *ifi = (ifinfomsg *)mnl_nlmsg_get_payload(n);
        parseAttributes(n, sizeof(*ifi), IFLA_MAX);
        handleLinkMessage(ifi, t == RTM_NEWLINK);
    }
    break;
    case RTM_NEWADDR:
        // fallthrough
    case RTM_DELADDR: {
        ifaddrmsg *ifa = (ifaddrmsg *)mnl_nlmsg_get_payload(n);
        parseAttributes(n, sizeof(*ifa), IFA_MAX);
        handleAddrMessage(ifa, t == RTM_NEWADDR);
    }
    break;
    default:
        LOG(WARNING) << "ignoring unexpected message type: " << t;
        break;
    }
    return MNL_CB_OK;
}

int RtNetlinkEthernetLinkAndIpAddressMonitor::mnlAttributeCallback(const nlattr *a)
{
    if (mnl_attr_type_valid(a, m_maxAttributeType) >= 0)
    {
        m_stats.parsedAttributes++;
        auto type = mnl_attr_get_type(a);
        m_lastSeenAttributes[type] = a;
    }
    return MNL_CB_OK;
}

NetworkInterfaceDescriptor &RtNetlinkEthernetLinkAndIpAddressMonitor::ensureNameAndIndexCurrent(int interfaceIndex)
{
    auto &cacheEntry = m_cache[interfaceIndex];
    cacheEntry.setIndex(interfaceIndex);
    if (cacheEntry.hasName())
    {
        // sometimes interfaces are renamed, account for that
        if (m_lastSeenAttributes[IFLA_IFNAME])
        {
            auto nameFromAttributes{mnl_attr_get_str(m_lastSeenAttributes[IFLA_IFNAME])};
            cacheEntry.setName(nameFromAttributes);
        }
        return cacheEntry;
    }
    // it's safe to use IFLA_IFNAME here as IFA_LABEL is the same id
    // we should be receiving link messages as first anyway
    if (m_lastSeenAttributes[IFLA_IFNAME])
    {
        VLOG(2) << "using name from already parsed attributes";
        m_stats.resolveIfNameByAttributes++;
        cacheEntry.setName(mnl_attr_get_str(m_lastSeenAttributes[IFLA_IFNAME]));
    }
    else
    {
        std::vector<char> namebuf(IF_NAMESIZE, '\0');
        auto name = if_indextoname(interfaceIndex, &namebuf[0]);
        if (name)
        {
            VLOG(2) << "resolved name via if_indextoname";
            m_stats.resolveIfNameByIfIndexToName++;
            cacheEntry.setName(name);
        }
        else
        {
            LOG(WARNING) << "failed to determine interface name from index " << interfaceIndex << "\n";
        }
    }
    return cacheEntry;
}

void RtNetlinkEthernetLinkAndIpAddressMonitor::handleLinkMessage(const ifinfomsg *ifi, bool isNew)
{
    if (ifi->ifi_type != ARPHRD_ETHER)
    {
        VLOG(4) << "ignoring non ethernet network interface with index: " << ifi->ifi_index;
        m_stats.msgsDiscarded++;
        return;
    }
    m_stats.linkUpdatesProcessed++;
    auto &cacheEntry = ensureNameAndIndexCurrent(ifi->ifi_index);
    if (isNew)
    {
        if (m_lastSeenAttributes[IFLA_CARRIER])
        {
            VLOG(3) << "carrier " << static_cast<int>(mnl_attr_get_u8(m_lastSeenAttributes[IFLA_CARRIER]));
        }
        if (m_lastSeenAttributes[IFLA_IFALIAS])
        {
            VLOG(3) << "ifalias " << mnl_attr_get_str(m_lastSeenAttributes[IFLA_IFALIAS]);
        }
        if (m_lastSeenAttributes[IFLA_OPERSTATE])
        {
            auto operstate{mnl_attr_get_u16(m_lastSeenAttributes[IFLA_OPERSTATE])};
            cacheEntry.setOperationalState(static_cast<OperationalState>(operstate));
        }
        if (m_lastSeenAttributes[IFLA_ADDRESS])
        {
            std::ostringstream strm;
            const uint8_t *hwaddr = (const uint8_t *)mnl_attr_get_payload(m_lastSeenAttributes[IFLA_ADDRESS]);
            const auto len = mnl_attr_get_payload_len(m_lastSeenAttributes[IFLA_ADDRESS]);
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
    }
    else
    {
        VLOG(2) << "removing interface " << cacheEntry.name();
        m_cache.erase(ifi->ifi_index);
    }
    printStatsForNerds();
}

void RtNetlinkEthernetLinkAndIpAddressMonitor::handleAddrMessage(const ifaddrmsg *ifa, bool isNew)
{
    if (m_cache.find(ifa->ifa_index) == m_cache.cend())
    {
        VLOG(4) << "ignoring unkown network interface with index: " << ifa->ifa_index;
        m_stats.msgsDiscarded++;
        return;
    }
    m_stats.addressUpdatesProcessed++;
    auto &cacheEntry = ensureNameAndIndexCurrent(ifa->ifa_index);
    if (m_lastSeenAttributes[IFA_PROTO])
    {
        auto proto{mnl_attr_get_u8(m_lastSeenAttributes[IFA_PROTO])};
        VLOG(3) << "proto " << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(proto);
    }
    if (m_lastSeenAttributes[IFA_FLAGS])
    {
        auto flags{mnl_attr_get_u32(m_lastSeenAttributes[IFA_FLAGS])};
        VLOG(3) << "flags " << std::setw(8) << std::setfill('0') << std::hex << flags;
    }
    if (m_lastSeenAttributes[IFA_ADDRESS])
    {
        void *addr = mnl_attr_get_payload(m_lastSeenAttributes[IFA_ADDRESS]);
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
            if (isNew)
            {
                cacheEntry.addAddress(strm.str());
            }
            else
            {
                cacheEntry.delAddress(strm.str());
            }
        }
    }
    printStatsForNerds();
}

void RtNetlinkEthernetLinkAndIpAddressMonitor::parseAttributes(const nlmsghdr *n, size_t offset, uint16_t maxtype)
{
    m_maxAttributeType = maxtype;
    std::fill(m_lastSeenAttributes.begin(), m_lastSeenAttributes.end(), nullptr);
    mnl_attr_parse(n, offset, &RtNetlinkEthernetLinkAndIpAddressMonitor::dispatchMnlAttributeCallbackToSelf, this);
}

void RtNetlinkEthernetLinkAndIpAddressMonitor::printStatsForNerds()
{
    if (m_cacheState != CacheState::WaitForChanges)
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
    VLOG(1) << "seen                " << m_stats.parsedAttributes << " attributes";
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

int RtNetlinkEthernetLinkAndIpAddressMonitor::dispatchMnlAttributeCallbackToSelf(const nlattr *a, void *self)
{
    return reinterpret_cast<RtNetlinkEthernetLinkAndIpAddressMonitor *>(self)->mnlAttributeCallback(a);
}
