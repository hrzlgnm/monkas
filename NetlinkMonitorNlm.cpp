#include "NetlinkMonitorNlm.h"

#include <arpa/inet.h>
#include <iostream>
#include <linux/if_link.h>
#include <linux/rtnetlink.h>
#include <memory.h>
#include <net/if.h>
#include <thread>
#include <sstream>
#include <iomanip>

#include <glog/logging.h>

namespace {
unsigned toNetlinkMulticastFlag(rtnetlink_groups group) {
    return (1 << (group - 1));
}
}

NetlinkMonitorNlm::NetlinkMonitorNlm()
    : m_mnlSocket{mnl_socket_open(NETLINK_ROUTE), mnl_socket_close},
      m_portid{mnl_socket_get_portid(m_mnlSocket.get())}, m_sequenceCounter{} {
    m_buffer.resize(MNL_SOCKET_BUFFER_SIZE);
    m_lastSeenAttributes.resize(IFLA_MAX + 1);
    unsigned groups = toNetlinkMulticastFlag(RTNLGRP_LINK);
    groups |= toNetlinkMulticastFlag(RTNLGRP_IPV4_IFADDR);
    groups |= toNetlinkMulticastFlag(RTNLGRP_IPV6_IFADDR);
    VLOG(1)<< "joining to RTnetlink multiacst groups "  << std::hex << std::setfill('0') << std::setw(8) << groups;
    if (mnl_socket_bind(m_mnlSocket.get(), groups, MNL_SOCKET_AUTOPID) < 0) {
        PLOG(FATAL) << "mnl_socket_bind";
    }
}

int NetlinkMonitorNlm::run() {
    LOG(INFO)<< "requesting RTM_GETLINK";
    request(RTM_GETLINK);
    startReceiving();
    return 0;
}

void NetlinkMonitorNlm::startReceiving() {
    auto ret = mnl_socket_recvfrom(m_mnlSocket.get(), &m_buffer[0], m_buffer.size());
    while (ret > 0) {
        m_stats.packetsReceived++;
        m_stats.bytesReceived += ret;
        if (VLOG_IS_ON(5)) {
            fflush(stderr);
            fflush(stdout);
            mnl_nlmsg_fprintf(stdout, &m_buffer[0], ret, 0);
        }
        ret = mnl_cb_run(&m_buffer[0], ret, m_sequenceCounter, m_portid,
                &NetlinkMonitorNlm::dipatchMnlDataCallbackToSelf, this);
        if (ret < MNL_CB_STOP) {
            LOG(WARNING) << "bork";
            break;
        }
        if (ret == MNL_CB_STOP) {
            if (m_cacheState == CacheState::FillLinkInfo) {
                m_cacheState = CacheState::FillAddressInfo;
                LOG(INFO)<< "requesting RTM_GETADDR";
                request(RTM_GETADDR);
            } else if (m_cacheState == CacheState::FillAddressInfo) {
                m_cacheState = CacheState::WaitForChanges;
                m_sequenceCounter = 0; // stop sequence checks
                LOG(INFO) << "cache filled with all available information successfully";
                LOG(INFO) << "tracking changes for: "<< m_cache.size() << " interfaces";
                dumpState();
            }
        }
        ret = mnl_socket_recvfrom(m_mnlSocket.get(), &m_buffer[0], m_buffer.size());
    }
}

void NetlinkMonitorNlm::request(uint16_t msgType) {
    ++m_sequenceCounter;
    nlmsghdr *nlh = mnl_nlmsg_put_header(&m_buffer[0]);
    nlh->nlmsg_type = msgType;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nlh->nlmsg_seq = m_sequenceCounter;
    rtgenmsg *rt =(rtgenmsg *)mnl_nlmsg_put_extra_header(nlh, sizeof(struct rtgenmsg));
    rt->rtgen_family = AF_PACKET;
    auto ret = mnl_socket_sendto(m_mnlSocket.get(), nlh, nlh->nlmsg_len);
    if (ret < 0) {
        PLOG(FATAL) << "mnl_socket_sendto";
    }
    m_stats.packetsSent++;
    m_stats.bytesSent+=ret;
}

int NetlinkMonitorNlm::mnlMessageCallback(const nlmsghdr *n) {
    m_stats.msgsReceived++;
    const auto t = n->nlmsg_type;
    switch (t) {
    case RTM_NEWLINK:
        // fallthrough
    case RTM_DELLINK:
    {
        ifinfomsg *ifi = (ifinfomsg *)mnl_nlmsg_get_payload(n);
        parseAttributes(n, sizeof(*ifi), IFLA_MAX);
        handleLinkMessage(ifi, t == RTM_NEWLINK);
    }
        break;
    case RTM_NEWADDR:
        // fallthrough
    case RTM_DELADDR:
    {
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

int NetlinkMonitorNlm::mnlAttributeCallback(const nlattr *a) {
    if (mnl_attr_type_valid(a, m_maxType) >= 0) {
        m_stats.parsedAttributes++;
        auto type = mnl_attr_get_type(a);
        m_lastSeenAttributes[type] = a;
    }
    return MNL_CB_OK;
}

NetworkInterfaceDescriptor &NetlinkMonitorNlm::ensureNamed(int interface_index) {
    auto &cacheEntry = m_cache[interface_index];
    if (cacheEntry.has_name()) {
        return cacheEntry;
    }
    if (m_lastSeenAttributes[IFLA_IFNAME]) {
        cacheEntry.set_name(mnl_attr_get_str(m_lastSeenAttributes[IFLA_IFNAME]));
    } else {
        std::vector<char> namebuf(IF_NAMESIZE, '\0');
        auto name = if_indextoname(interface_index, &namebuf[0]);
        if (name) {
            cacheEntry.set_name(name);
        } else {
            LOG(WARNING) << "failed to determine interface name from index "
                         << interface_index << "\n";
        }
    }
    return cacheEntry;
}

void NetlinkMonitorNlm::handleLinkMessage(const ifinfomsg *ifi, bool isNew) {
    auto& cacheEntry = ensureNamed(ifi->ifi_index);;
    if (m_lastSeenAttributes[IFLA_OPERSTATE]) {
        cacheEntry.set_operstate(static_cast<NetworkInterfaceDescriptor::OperState>(mnl_attr_get_u16(m_lastSeenAttributes[IFLA_OPERSTATE])));
    }
    if (m_lastSeenAttributes[IFLA_ADDRESS]) {
        if (isNew) {
            std::ostringstream strm;
            strm << "hw=";
            const uint8_t *hwaddr = (const uint8_t*) mnl_attr_get_payload(m_lastSeenAttributes[IFLA_ADDRESS]);
            const auto len = mnl_attr_get_payload_len(m_lastSeenAttributes[IFLA_ADDRESS]);
            for (int i=0; i<len; i++) {
                strm << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hwaddr[i]);
                if(i+1 != len) {
                    strm << ':';
                }
            }
            cacheEntry.push_addr(strm.str());
        }
    }
    dumpState();
}

void NetlinkMonitorNlm::handleAddrMessage(const ifaddrmsg *ifa, bool isNew) {
    auto& cacheEntry = ensureNamed(ifa->ifa_index);

    /* for point to point local is local, address is peer*/
    if (!m_lastSeenAttributes[IFA_LOCAL])
        m_lastSeenAttributes[IFA_LOCAL] = m_lastSeenAttributes[IFA_ADDRESS];
    if (!m_lastSeenAttributes[IFA_ADDRESS])
        m_lastSeenAttributes[IFA_ADDRESS] = m_lastSeenAttributes[IFA_LOCAL];

    if (m_lastSeenAttributes[IFA_ADDRESS]) {
        void *addr = mnl_attr_get_payload(m_lastSeenAttributes[IFA_ADDRESS]);
        void *laddr = mnl_attr_get_payload(m_lastSeenAttributes[IFA_LOCAL]);
        const bool is_peer = !!memcmp(addr, laddr, ifa->ifa_family == AF_INET ? 4 : 16);
        char out[INET6_ADDRSTRLEN];
        if (inet_ntop(ifa->ifa_family, addr, out, sizeof(out))) {
            std::stringstream strm;
            if (is_peer) {
                strm << "peer:";
            }
            if (ifa->ifa_family == AF_INET6) {
                strm << "ip6=";
            } else if (ifa->ifa_family == AF_INET) {
                strm << "ip4=";
            }
            strm << out << "/" << static_cast<int>(ifa->ifa_prefixlen);
            switch(ifa->ifa_scope) {
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
            if (isNew) {
                cacheEntry.push_addr(strm.str());
            } else {
                cacheEntry.del_addr(strm.str());
            }
        }
    }
    dumpState();
}

void NetlinkMonitorNlm::parseAttributes(const nlmsghdr *n, size_t offset, uint16_t maxtype) {
    m_maxType = maxtype;
    std::fill(m_lastSeenAttributes.begin(), m_lastSeenAttributes.end(), nullptr);
    mnl_attr_parse(n, offset, &NetlinkMonitorNlm::dispatchMnlAttributeCallbackToSelf, this);
}

void NetlinkMonitorNlm::dumpState() {
    if (m_cacheState != CacheState::WaitForChanges) {
        return;
    }
    LOG(INFO) << "sent:     " << m_stats.bytesSent << " bytes";
    LOG(INFO) << "sent:     " << m_stats.packetsSent << " packets";
    LOG(INFO) << "received: " << m_stats.packetsReceived << " pakets";
    LOG(INFO) << "received: " << m_stats.bytesReceived << " bytes";
    LOG(INFO) << "received: " << m_stats.msgsReceived<< " netlink messages";
    LOG(INFO) << "parsed:   " << m_stats.parsedAttributes<< " attributes";
    LOG(INFO) << "interface details by index:";
    for (const auto &c : m_cache) {
        LOG(INFO) << '\t' << c.first << ": " << c.second;
    }
}

int NetlinkMonitorNlm::dipatchMnlDataCallbackToSelf(const nlmsghdr *n, void *self) {
    return reinterpret_cast<NetlinkMonitorNlm *>(self)->mnlMessageCallback(n);
}

int NetlinkMonitorNlm::dispatchMnlAttributeCallbackToSelf(const nlattr *a, void *self) {
    return reinterpret_cast<NetlinkMonitorNlm *>(self)->mnlAttributeCallback(a);
}

NetworkInterfaceDescriptor::NetworkInterfaceDescriptor()
    : m_operstate{},
      m_lastChanged(std::chrono::steady_clock::now()) {

}

bool NetworkInterfaceDescriptor::has_name() const
{
    return !m_name.empty();
}

const std::string &NetworkInterfaceDescriptor::name() const { return m_name; }

void NetworkInterfaceDescriptor::set_name(const std::string &name) {
    if (m_name != name) {
        m_lastChanged = std::chrono::steady_clock::now();
        m_name = name;
        VLOG(1) << this << " " << "name changed to: " << name << "\n";
    }
}

NetworkInterfaceDescriptor::OperState NetworkInterfaceDescriptor::operstate() const {
    return m_operstate;
}

void NetworkInterfaceDescriptor::set_operstate(OperState operstate) {
    if (m_operstate != operstate) {
        m_lastChanged = std::chrono::steady_clock::now();
        m_operstate = operstate;
        VLOG(1) << this << " " << m_name << " operstate changed to: " << static_cast<int>(operstate) << "\n";
    }
}


void NetworkInterfaceDescriptor::push_addr(const std::string &addr) {
    auto res = m_addresses.emplace(addr);
    if (res.second) {
        m_lastChanged = std::chrono::steady_clock::now();
        VLOG(1) << this << " " << m_name << " addr added: " << addr << "\n";
    }
}

void NetworkInterfaceDescriptor::del_addr(const std::string &addr) {
    auto res = m_addresses.erase(addr);
    if (res > 0) {
        m_lastChanged = std::chrono::steady_clock::now();
        VLOG(1)  << this << " " << m_name << " addr removed: " << addr << "\n";
    }
}

age_duration NetworkInterfaceDescriptor::age() const
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_lastChanged);
}

std::string to_string(NetworkInterfaceDescriptor::OperState o) {
    using OperState = NetworkInterfaceDescriptor::OperState;
    switch(o) {
    case OperState::NotPresent: return "NotPresent";
    case OperState::Down: return "Down";
    case OperState::LowerLayerDown: return "LowerLayerDown";
    case OperState::Testing: return "Testing";
    case OperState::Dormant: return "Dormant";
    case OperState::Up: return "Up";
    case OperState::Unknown:
        // fallthrough
    default:
        return "Uknown";
    }
}

std::ostream &operator<<(std::ostream &o, const NetworkInterfaceDescriptor &s) {
    o << s.name();
    for (const auto &a : s.m_addresses) {
        o << " " << a;
    }
    o << " state=" << to_string(s.m_operstate) << "(" << static_cast<int>(s.m_operstate) << ")";
    o << " age=" << s.age().count();
    return o;
}
