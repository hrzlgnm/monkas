#include <ip/Address.hpp>
#include <monitor/RtnlNetworkMonitor.hpp>

#include <libmnl/libmnl.h>
#include <linux/rtnetlink.h>
#include <memory.h>
#include <net/if_arp.h>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>

namespace spdlog
{
template <typename T> [[noreturn]] void pfatal(const T &msg)
{
    spdlog::flush_on(spdlog::level::critical);
    spdlog::critical("{} failed: {}[{}]", msg, strerror(errno), errno);
    std::abort();
}

template <typename T> void pwarn(const T &msg)
{
    spdlog::warn("{} failed: {}[{}]", msg, strerror(errno), errno);
}

} // namespace spdlog

namespace
{
unsigned toRtnlGroupFlag(rtnetlink_groups group)
{
    return (1 << (group - 1));
}

inline constexpr size_t SOCKET_BUFFER_SIZE = 32 * 1024;

static mnl_socket *ensure_mnl_socket(bool nonBlocking)
{
    auto s = mnl_socket_open2(NETLINK_ROUTE, nonBlocking ? SOCK_NONBLOCK : 0);
    if (!s)
    {
        spdlog::pfatal("mnl_socket_open2");
    }
    return s;
}
} // namespace

namespace monkas
{

RtnlNetworkMonitor::RtnlNetworkMonitor(const RuntimeOptions &options)
    : m_mnlSocket{ensure_mnl_socket(options & NonBlocking), mnl_socket_close}
    , m_portid{mnl_socket_get_portid(m_mnlSocket.get())}
    , m_buffer(SOCKET_BUFFER_SIZE)
    , m_runtimeOptions(options)
{
    m_stats.startTime = std::chrono::steady_clock::now();
    unsigned groups = toRtnlGroupFlag(RTNLGRP_LINK);
    groups |= toRtnlGroupFlag(RTNLGRP_NOTIFY);
    if (!(m_runtimeOptions & RuntimeFlag::PreferredFamilyV6))
    {
        groups |= toRtnlGroupFlag(RTNLGRP_IPV4_IFADDR);
        groups |= toRtnlGroupFlag(RTNLGRP_IPV4_ROUTE);
    }
    if (!(m_runtimeOptions & RuntimeFlag::PreferredFamilyV4))
    {
        groups |= toRtnlGroupFlag(RTNLGRP_IPV6_IFADDR);
        groups |= toRtnlGroupFlag(RTNLGRP_IPV6_ROUTE);
    }
    spdlog::debug("Joining RTnetlink multicast groups {}", groups);
    if (mnl_socket_bind(m_mnlSocket.get(), groups, MNL_SOCKET_AUTOPID) < 0)
    {
        spdlog::pfatal("mnl_socket_bind");
    }
}

void RtnlNetworkMonitor::enumerateInterfaces()
{
    if (m_cacheState == CacheState::WaitingForChanges)
    {
        spdlog::trace("Already enumerated interfaces, skipping");
        return;
    }
    spdlog::debug("Requesting RTM_GETLINK");
    sendDumpRequest(RTM_GETLINK);
    while (m_cacheState != CacheState::WaitingForChanges)
    {
        receiveAndProcess();
    }
}

int RtnlNetworkMonitor::run()
{
    // someone may call enumerateInterfaces() and stop() during enumerateInterfaces
    if (!m_mnlSocket)
    {
        return 0;
    }
    m_running = true;
    spdlog::trace("Starting RtnlNetworkMonitor");
    enumerateInterfaces();
    spdlog::trace("Starting to receive and process messages from mnl socket {} m_running={}",
                  static_cast<void *>(m_mnlSocket.get()), m_running);
    while (m_running)
    {
        receiveAndProcess();
    }
    return 0;
}

void RtnlNetworkMonitor::stop()
{
    spdlog::debug("Stopping RtnlNetworkMonitor");
    m_mnlSocket.reset();
    m_running = false;
}

InterfacesWatcherToken RtnlNetworkMonitor::addInterfacesWatcher(const InterfacesWatcher &watcher, bool initialSnapshot)
{
    if (initialSnapshot)
    {
        watcher(getInterfacesSnapshot());
    }
    return m_interfacesNotifier.addWatcher(watcher);
}

void RtnlNetworkMonitor::removeInterfacesWatcher(const InterfacesWatcherToken &token)
{
    m_interfacesNotifier.removeWatcher(token);
}

OperationalStateWatcherToken RtnlNetworkMonitor::addOperationalStateWatcher(const OperationalStateWatcher &watcher,
                                                                            bool initialSnapshot)
{
    if (initialSnapshot)
    {
        for (auto &[index, tracker] : m_trackers)
        {
            watcher(network::Interface{index, tracker.name()}, tracker.operationalState());
            tracker.clearFlag(DirtyFlag::OperationalStateChanged);
        }
    }
    return m_operationalStateNotifier.addWatcher(watcher);
}

void RtnlNetworkMonitor::removeOperationalStateWatcher(const OperationalStateWatcherToken &token)
{
    m_operationalStateNotifier.removeWatcher(token);
}

NetworkAddressWatcherToken RtnlNetworkMonitor::addNetworkAddressWatcher(const NetworkAddressWatcher &watcher,
                                                                        bool initialSnapshot)
{
    if (initialSnapshot)
    {
        for (auto &[index, tracker] : m_trackers)
        {
            watcher(network::Interface{index, tracker.name()}, tracker.networkAddresses());
            tracker.clearFlag(DirtyFlag::NetworkAddressesChanged);
        }
    }
    return m_networkAddressNotifier.addWatcher(watcher);
}

void RtnlNetworkMonitor::removeNetworkAddressWatcher(const NetworkAddressWatcherToken &token)
{
    m_networkAddressNotifier.removeWatcher(token);
}

GatewayAddressWatcherToken RtnlNetworkMonitor::addGatewayAddressWatcher(const GatewayAddressWatcher &watcher,
                                                                        bool initialSnapshot)
{
    if (initialSnapshot)
    {
        for (auto &[index, tracker] : m_trackers)
        {
            watcher(network::Interface{index, tracker.name()}, tracker.gatewayAddress());
            tracker.clearFlag(DirtyFlag::GatewayAddressChanged);
        }
    }
    return m_gatewayAddressNotifier.addWatcher(watcher);
}

void RtnlNetworkMonitor::removeGatewayAddressWatcher(const GatewayAddressWatcherToken &token)
{
    m_gatewayAddressNotifier.removeWatcher(token);
}

EthernetAddressWatcherToken RtnlNetworkMonitor::addMacAddressWatcher(const EthernetAddressWatcher &watcher,
                                                                     bool initialSnapshot)
{
    if (initialSnapshot)
    {
        for (auto &[index, tracker] : m_trackers)
        {
            watcher(network::Interface{index, tracker.name()}, tracker.ethernetAddress());
            tracker.clearFlag(DirtyFlag::EthernetAddressChanged);
        }
    }
    return m_macAddressNotifier.addWatcher(watcher);
}

void RtnlNetworkMonitor::removeMacAddressWatcher(const EthernetAddressWatcherToken &token)
{
    m_macAddressNotifier.removeWatcher(token);
}

EthernetAddressWatcherToken RtnlNetworkMonitor::addBroadcastAddressWatcher(const EthernetAddressWatcher &watcher,
                                                                           bool initialSnapshot)
{
    if (initialSnapshot)
    {
        for (auto &[index, tracker] : m_trackers)
        {
            watcher(network::Interface{index, tracker.name()}, tracker.broadcastAddress());
            tracker.clearFlag(DirtyFlag::BroadcastAddressChanged);
        }
    }
    return m_broadcastAddressNotifier.addWatcher(watcher);
}

void RtnlNetworkMonitor::removeBroadcastAddressWatcher(const EthernetAddressWatcherToken &token)
{
    m_broadcastAddressNotifier.removeWatcher(token);
}

std::optional<EnumerationDoneWatcherToken> RtnlNetworkMonitor::addEnumerationDoneWatcher(
    const EnumerationDoneWatcher &watcher)
{
    if (m_cacheState == CacheState::WaitingForChanges)
    {
        watcher();
        return std::nullopt; // already done with enumeration, no need to notify later
    }
    return m_enumerationDoneNotifier.addWatcher(watcher);
}

void RtnlNetworkMonitor::removeEnumerationDoneWatcher(const EnumerationDoneWatcherToken &token)
{
    m_enumerationDoneNotifier.removeWatcher(token);
}

void RtnlNetworkMonitor::receiveAndProcess()
{
    // someone may call stop() while we are notifying watchers
    if (!m_mnlSocket)
    {
        return;
    }
    spdlog::trace("Receiving messages from mnl socket");
    auto receiveResult = mnl_socket_recvfrom(m_mnlSocket.get(), &m_buffer[0], m_buffer.size());
    spdlog::trace("Received {} bytes", receiveResult);
    while (receiveResult > 0)
    {
        m_stats.packetsReceived++;
        m_stats.bytesReceived += receiveResult;
        if (m_runtimeOptions & DumpPackets)
        {
            fflush(stderr);
            fflush(stdout);
            mnl_nlmsg_fprintf(stdout, &m_buffer[0], receiveResult, 0);
        }
        const auto seqNo = isEnumerating() ? m_sequenceNumber : 0;
        const auto callbackResult = mnl_cb_run(&m_buffer[0], receiveResult, seqNo, m_portid,
                                               &RtnlNetworkMonitor::dispatchMnMessageCallbackToSelf, this);
        if (callbackResult == MNL_CB_ERROR)
        {
            spdlog::pwarn("mnl_cb_run");
            break;
        }
        if (callbackResult == MNL_CB_STOP)
        {
            if (isEnumeratingLinks())
            {
                m_cacheState = CacheState::EnumeratingAddresses;
                spdlog::debug("Requesting RTM_GETADDR");
                sendDumpRequest(RTM_GETADDR);
            }
            else if (isEnumeratingAddresses())
            {
                m_cacheState = CacheState::EnumeratingRoutes;
                spdlog::debug("Requesting RTM_GETROUTE");
                sendDumpRequest(RTM_GETROUTE);
            }
            else if (isEnumeratingRoutes())
            {
                m_cacheState = CacheState::WaitingForChanges;
                spdlog::debug("Done with enumeration of initial information");
                spdlog::debug("Tracking changes for {} interfaces", m_trackers.size());
                if (m_enumerationDoneNotifier.hasWatchers())
                {
                    m_enumerationDoneNotifier.notify();
                }
                // run() will restart this loop
                break;
            }
            else
            {
                if (m_mnlSocket)
                {
                    spdlog::pwarn("Unexpected MNL_CB_STOP");
                }
                else
                {
                    break; // someone may call stop() while we are notifying watchers
                }
            }
        }
        printStatsForNerdsIfEnabled();
        notifyChanges();
        // someone may call stop() while we are notifying watchers
        if (m_mnlSocket)
        {
            receiveResult = mnl_socket_recvfrom(m_mnlSocket.get(), &m_buffer[0], m_buffer.size());
        }
    }
}

void RtnlNetworkMonitor::sendDumpRequest(uint16_t msgType)
{
    nlmsghdr *nlh = mnl_nlmsg_put_header(&m_buffer[0]);
    nlh->nlmsg_type = msgType;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nlh->nlmsg_seq = ++m_sequenceNumber;
    rtgenmsg *gen = (rtgenmsg *)mnl_nlmsg_put_extra_header(nlh, sizeof(struct rtgenmsg));
    gen->rtgen_family = AF_UNSPEC;
    mnl_attr_put_u32(nlh, IFLA_EXT_MASK, RTEXT_FILTER_SKIP_STATS);
    auto ret = mnl_socket_sendto(m_mnlSocket.get(), nlh, nlh->nlmsg_len);
    if (ret < 0)
    {
        spdlog::pfatal("mnl_socket_sendto");
    }
    m_stats.packetsSent++;
    m_stats.bytesSent += ret;
}

int RtnlNetworkMonitor::mnlMessageCallback(const nlmsghdr *n)
{
    if (!m_mnlSocket)
    {
        return MNL_CB_STOP; // someone may call stop() while we are processing messages
    }
    m_stats.msgsReceived++;
    const auto t = n->nlmsg_type;
    switch (t)
    {
    case RTM_NEWLINK:
    case RTM_DELLINK: {
        const ifinfomsg *ifi = reinterpret_cast<const ifinfomsg *>(mnl_nlmsg_get_payload(n));
        parseLinkMessage(n, ifi);
    }
    break;
    case RTM_NEWADDR:
    case RTM_DELADDR: {
        const ifaddrmsg *ifa = reinterpret_cast<const ifaddrmsg *>(mnl_nlmsg_get_payload(n));
        parseAddressMessage(n, ifa);
    }
    break;
    case RTM_NEWROUTE:
    case RTM_DELROUTE: {
        const rtmsg *rt = reinterpret_cast<const rtmsg *>(mnl_nlmsg_get_payload(n));
        parseRouteMessage(n, rt);
    }
    break;
    default:
        spdlog::warn("ignoring unexpected message type: {}", t);
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
        spdlog::warn("ignoring unexpected nlattr type {}", type);
    }
}

NetworkInterfaceStatusTracker &RtnlNetworkMonitor::ensureNameCurrent(int ifIndex, const nlattr *nameAttribute)
{
    const auto before = m_trackers.size();
    auto &cacheEntry = m_trackers[ifIndex];

    // Sometimes interfaces are renamed, account for that
    if (nameAttribute)
    {
        cacheEntry.setName(mnl_attr_get_str(nameAttribute));
    }
    if (before != m_trackers.size() || cacheEntry.isDirty(DirtyFlag::NameChanged))
    {
        notifyInterfacesSnapshot();
        cacheEntry.clearFlag(DirtyFlag::NameChanged);
    }
    return cacheEntry;
}

void RtnlNetworkMonitor::parseLinkMessage(const nlmsghdr *nlhdr, const ifinfomsg *ifi)
{
    m_stats.linkMessagesSeen++;
    // TODO ifi_type filters
    if (ifi->ifi_type != ARPHRD_ETHER)
    {
        m_stats.msgsDiscarded++;
        return;
    }
    if (nlhdr->nlmsg_type == RTM_DELLINK)
    {
        spdlog::trace("removing interface with index {}", ifi->ifi_index);
        m_trackers.erase(ifi->ifi_index);
        notifyInterfacesSnapshot();
        return;
    }
    auto attributes = parseAttributes(nlhdr, sizeof(*ifi), IFLA_MAX);

    auto &cacheEntry = ensureNameCurrent(ifi->ifi_index, attributes[IFLA_IFNAME]);
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
}

void RtnlNetworkMonitor::parseAddressMessage(const nlmsghdr *nlhdr, const ifaddrmsg *ifa)
{
    m_stats.addressMessagesSeen++;
    if (m_trackers.find(ifa->ifa_index) == m_trackers.cend())
    {
        m_stats.msgsDiscarded++;
        return;
    }
    if (m_runtimeOptions & RuntimeFlag::PreferredFamilyV4 && ifa->ifa_family != AF_INET)
    {
        m_stats.msgsDiscarded++;
        return;
    }

    if (m_runtimeOptions & RuntimeFlag::PreferredFamilyV6 && ifa->ifa_family != AF_INET6)
    {
        m_stats.msgsDiscarded++;
        return;
    }

    auto attributes = parseAttributes(nlhdr, sizeof(*ifa), IFA_MAX);

    uint32_t flags = ifa->ifa_flags; // will be overwritten if IFA_FLAGS is present
    ip::Address address;
    ip::Address broadcast;

    auto &cacheEntry = ensureNameCurrent(ifa->ifa_index, attributes[IFA_LABEL]);
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
        address.addressFamily(), address, broadcast, ifa->ifa_prefixlen, network::fromRtnlScope(ifa->ifa_scope), flags};
    if (nlhdr->nlmsg_type == RTM_NEWADDR)
    {
        cacheEntry.addNetworkAddress(networkAddress);
    }
    else if (nlhdr->nlmsg_type == RTM_DELADDR)
    {
        cacheEntry.removeNetworkAddress(networkAddress);
    }
}

void RtnlNetworkMonitor::parseRouteMessage(const nlmsghdr *nlhdr, const rtmsg *rtm)
{
    m_stats.routeMessagesSeen++;
    if (rtm->rtm_family != AF_INET)
    {
        m_stats.msgsDiscarded++;
        return;
    }
    if (m_runtimeOptions & RuntimeFlag::PreferredFamilyV6 && rtm->rtm_family != AF_INET6)
    {
        m_stats.msgsDiscarded++;
        return;
    }

    auto attributes = parseAttributes(nlhdr, sizeof(*rtm), RTA_MAX);
    if (nlhdr->nlmsg_type == RTM_DELROUTE)
    {
        // remove ipv4 routing default gateway when a linkdown was detected for the interface
        if (rtm->rtm_flags & RTNH_F_LINKDOWN && attributes[RTA_OIF])
        {
            auto outIfIndex = mnl_attr_get_u32(attributes[RTA_OIF]);
            auto itr = m_trackers.find(outIfIndex);
            if (itr != m_trackers.end() && itr->second.gatewayAddress())
            {
                itr->second.clearGatewayAddress(GatewayClearReason::LinkDown);
            }
        }
        else if (attributes[RTA_GATEWAY] && attributes[RTA_OIF])
        {
            auto outIfIndex = mnl_attr_get_u32(attributes[RTA_OIF]);
            auto itr = m_trackers.find(outIfIndex);
            if (itr != m_trackers.end())
            {
                itr->second.clearGatewayAddress(GatewayClearReason::RouteDeleted);
            }
        }
        return;
    }
    if (attributes[RTA_GATEWAY] && attributes[RTA_OIF])
    {
        auto outif = mnl_attr_get_u32(attributes[RTA_OIF]);
        auto itr = m_trackers.find(outif);
        if (itr != m_trackers.end())
        {
            const uint8_t *addr = (const uint8_t *)mnl_attr_get_payload(attributes[RTA_GATEWAY]);
            auto len = mnl_attr_get_payload_len(attributes[RTA_GATEWAY]);
            itr->second.setGatewayAddress(ip::Address::fromBytes(addr, len));
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

void RtnlNetworkMonitor::printStatsForNerdsIfEnabled()
{
    if (isEnumerating() || !(m_runtimeOptions & RuntimeFlag::StatsForNerds))
    {
        return;
    }
    spdlog::info("--------------- Stats for nerds -----------------");
    spdlog::info("uptime    {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(
                                       std::chrono::steady_clock::now() - m_stats.startTime)
                                       .count());
    spdlog::info("sent      {} bytes in {} packets", m_stats.bytesSent, m_stats.packetsSent);
    spdlog::info("received  {} bytes in {} packets", m_stats.bytesReceived, m_stats.packetsReceived);
    spdlog::info("received  {} rtnl messages", m_stats.msgsReceived);
    spdlog::info("discarded {} rtnl messages", m_stats.msgsDiscarded);
    spdlog::info("* seen");
    spdlog::info("          {} attribute entries", m_stats.seenAttributes);
    spdlog::info("          {} link messages", m_stats.linkMessagesSeen);
    spdlog::info("          {} address messages", m_stats.addressMessagesSeen);
    spdlog::info("          {} route messages", m_stats.routeMessagesSeen);

    spdlog::info("----------- Interface details in cache ----------");
    for (const auto &c : m_trackers)
    {
        spdlog::info(c.second);
    }
    spdlog::info("-------------------------------------------------");
}

int RtnlNetworkMonitor::dispatchMnMessageCallbackToSelf(const nlmsghdr *n, void *self)
{
    return reinterpret_cast<RtnlNetworkMonitor *>(self)->mnlMessageCallback(n);
}

int RtnlNetworkMonitor::dispatchMnlAttributeCallback(const nlattr *a, void *ud)
{
    auto args = reinterpret_cast<MnlAttributeCallbackArgs *>(ud);
    parseAttribute(a, *args->maxType, *args->attrs, *args->counter);
    return MNL_CB_OK;
}

void RtnlNetworkMonitor::notifyChanges()
{
    for (auto &[index, tracker] : m_trackers)
    {
        spdlog::trace("checking {} for changes", tracker);
        if (m_operationalStateNotifier.hasWatchers() && tracker.isDirty(DirtyFlag::OperationalStateChanged))
        {
            m_operationalStateNotifier.notify(network::Interface{index, tracker.name()}, tracker.operationalState());
            tracker.clearFlag(DirtyFlag::OperationalStateChanged);
        }
        if (m_networkAddressNotifier.hasWatchers() && tracker.isDirty(DirtyFlag::NetworkAddressesChanged))
        {
            m_networkAddressNotifier.notify(network::Interface{index, tracker.name()}, tracker.networkAddresses());
            tracker.clearFlag(DirtyFlag::NetworkAddressesChanged);
        }
        if (m_gatewayAddressNotifier.hasWatchers() && tracker.isDirty(DirtyFlag::GatewayAddressChanged))
        {
            m_gatewayAddressNotifier.notify(network::Interface{index, tracker.name()}, tracker.gatewayAddress());
            tracker.clearFlag(DirtyFlag::GatewayAddressChanged);
        }
        if (m_macAddressNotifier.hasWatchers() && tracker.isDirty(DirtyFlag::EthernetAddressChanged))
        {
            m_macAddressNotifier.notify(network::Interface{index, tracker.name()}, tracker.ethernetAddress());
            tracker.clearFlag(DirtyFlag::EthernetAddressChanged);
        }
        if (m_broadcastAddressNotifier.hasWatchers() && tracker.isDirty(DirtyFlag::BroadcastAddressChanged))
        {
            m_broadcastAddressNotifier.notify(network::Interface{index, tracker.name()}, tracker.broadcastAddress());
            tracker.clearFlag(DirtyFlag::BroadcastAddressChanged);
        }
    }
}

void RtnlNetworkMonitor::notifyInterfacesSnapshot()
{
    if (m_interfacesNotifier.hasWatchers())
    {
        m_interfacesNotifier.notify(getInterfacesSnapshot());
    }
}

Interfaces RtnlNetworkMonitor::getInterfacesSnapshot() const
{
    Interfaces intfs;
    for (const auto &[idx, tracker] : m_trackers)
    {
        if (tracker.hasName())
        {
            intfs.emplace(idx, tracker.name());
        }
    }
    return intfs;
}

RuntimeOptions &operator|=(RuntimeOptions &o, RuntimeFlag f)
{
    o |= static_cast<RuntimeOptions>(f);
    return o;
}

RuntimeOptions operator&(RuntimeOptions o, RuntimeFlag f)
{
    return o & static_cast<RuntimeOptions>(f);
}

} // namespace monkas
