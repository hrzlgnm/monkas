#include <cerrno>
#include <cstddef>
#include <ranges>

#include <fmt/std.h>
#include <ip/Address.hpp>
#include <libmnl/libmnl.h>
#include <linux/if.h>
#include <linux/if_link.h>
#include <linux/rtnetlink.h>
#include <memory.h>
#include <monitor/Attributes.hpp>
#include <monitor/NetworkMonitor.hpp>
#include <net/if_arp.h>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>

#include "network/Address.hpp"

namespace monkas::monitor
{
namespace
{
template<typename T>
[[noreturn]] void pfatal(const T& msg)
{
    spdlog::flush_on(spdlog::level::critical);
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    spdlog::critical("{} failed: {}[{}]", msg, strerror(errno), errno);
    std::abort();
}

template<typename T>
void pwarn(const T& msg)
{
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    spdlog::warn("{} failed: {}[{}]", msg, strerror(errno), errno);
}

auto toRtnlGroupFlag(const rtnetlink_groups group) -> unsigned
{
    return 1U << (group - 1U);
}

constexpr auto RECEIVE_SOCKET_BUFFER_SIZE = 32U * 1024U;
constexpr auto SEND_SOCKET_BUFFER_SIZE = 4U * 1024U;

auto ensureMnlSocket(const bool nonBlocking) -> mnl_socket*
{
    auto* s = mnl_socket_open2(NETLINK_ROUTE, nonBlocking ? SOCK_NONBLOCK : 0);
    if (s == nullptr) {
        pfatal("mnl_socket_open2");
    }
    return s;
}
}  // namespace

NetworkMonitor::NetworkMonitor(const RuntimeFlags& options)
    : m_mnlSocket {ensureMnlSocket(options.test(RuntimeFlag::NonBlocking)), mnl_socket_close}
    , m_receiveBuffer(RECEIVE_SOCKET_BUFFER_SIZE)
    , m_sendBuffer(SEND_SOCKET_BUFFER_SIZE)
    , m_portid {mnl_socket_get_portid(m_mnlSocket.get())}
    , m_runtimeOptions(options)
{
    m_stats.startTime = std::chrono::steady_clock::now();
    unsigned groups = toRtnlGroupFlag(RTNLGRP_LINK);
    groups |= toRtnlGroupFlag(RTNLGRP_NOTIFY);
    if (!m_runtimeOptions.test(RuntimeFlag::PreferredFamilyV6)) {
        groups |= toRtnlGroupFlag(RTNLGRP_IPV4_IFADDR);
        groups |= toRtnlGroupFlag(RTNLGRP_IPV4_ROUTE);
    }
    if (!m_runtimeOptions.test(RuntimeFlag::PreferredFamilyV4)) {
        groups |= toRtnlGroupFlag(RTNLGRP_IPV6_IFADDR);
        groups |= toRtnlGroupFlag(RTNLGRP_IPV6_ROUTE);
    }
    spdlog::debug("Joining RTnetlink multicast groups {}", groups);
    if (mnl_socket_bind(m_mnlSocket.get(), groups, MNL_SOCKET_AUTOPID) < 0) {
        pfatal("mnl_socket_bind");
    }
}

/**
 * @brief Initiates enumeration of network interfaces and waits for completion.
 *
 * Sends a netlink dump request to retrieve all network interfaces and processes messages until enumeration is complete.
 * If enumeration has already been performed, the function returns immediately.
 */
void NetworkMonitor::enumerateInterfaces()
{
    if (m_cacheState == CacheState::WaitingForChanges) {
        spdlog::trace("Already enumerated interfaces, skipping");
        return;
    }
    spdlog::debug("Requesting RTM_GETLINK");
    sendDumpRequest(RTM_GETLINK);
    while (m_cacheState != CacheState::WaitingForChanges) {
        receiveAndProcess();
    }
}

/**
 * @brief Starts monitoring network interfaces and processes netlink messages until stopped.
 *
 * Initiates interface enumeration and then enters a loop to receive and process netlink messages, continuing until
 * monitoring is explicitly stopped.
 */
void NetworkMonitor::run()
{
    // someone may call enumerateInterfaces() and stop() during enumerateInterfaces
    if (!m_mnlSocket) {
        return;
    }
    m_running = true;
    spdlog::trace("Starting NetworkMonitor");
    enumerateInterfaces();
    spdlog::trace("Starting to receive and process messages from mnl socket {} m_running={}",
                  static_cast<void*>(m_mnlSocket.get()),
                  m_running);
    while (m_running) {
        receiveAndProcess();
    }
}

/**
 * @brief Stops the network monitoring process and releases resources.
 *
 * Closes the netlink socket and marks the monitor as no longer running.
 */
void NetworkMonitor::stop()
{
    spdlog::debug("Stopping NetworkMonitor");
    m_mnlSocket.reset();
    m_running = false;
}

auto NetworkMonitor::addInterfacesWatcher(const InterfacesWatcher& watcher, const InitialSnapshotMode initialSnapshot)
    -> InterfacesWatcherToken
{
    if (initialSnapshot == InitialSnapshotMode::InitialSnapshot) {
        watcher(getInterfacesSnapshot());
    }
    return m_interfacesNotifier.addWatcher(watcher);
}

void NetworkMonitor::removeInterfacesWatcher(const InterfacesWatcherToken& token)
{
    m_interfacesNotifier.removeWatcher(token);
}

auto NetworkMonitor::addLinkFlagsWatcher(const LinkFlagsWatcher& watcher, const InitialSnapshotMode initialSnapshot)
    -> LinkFlagsWatcherToken
{
    if (initialSnapshot == InitialSnapshotMode::InitialSnapshot) {
        for (auto& [index, tracker] : m_trackers) {
            watcher(network::Interface {index, tracker.name()}, tracker.linkFlags());
            tracker.clearFlag(DirtyFlag::LinkFlagsChanged);
        }
    }
    return m_linkFlagsNotifier.addWatcher(watcher);
}

void NetworkMonitor::removeLinkFlagsWatcher(const LinkFlagsWatcherToken& token)
{
    m_linkFlagsNotifier.removeWatcher(token);
}

auto NetworkMonitor::addOperationalStateWatcher(const OperationalStateWatcher& watcher,
                                                const InitialSnapshotMode initialSnapshot)
    -> OperationalStateWatcherToken
{
    if (initialSnapshot == InitialSnapshotMode::InitialSnapshot) {
        for (auto& [index, tracker] : m_trackers) {
            watcher(network::Interface {index, tracker.name()}, tracker.operationalState());
            tracker.clearFlag(DirtyFlag::OperationalStateChanged);
        }
    }
    return m_operationalStateNotifier.addWatcher(watcher);
}

void NetworkMonitor::removeOperationalStateWatcher(const OperationalStateWatcherToken& token)
{
    m_operationalStateNotifier.removeWatcher(token);
}

auto NetworkMonitor::addNetworkAddressWatcher(const NetworkAddressWatcher& watcher,
                                              const InitialSnapshotMode initialSnapshot) -> NetworkAddressWatcherToken
{
    if (initialSnapshot == InitialSnapshotMode::InitialSnapshot) {
        for (auto& [index, tracker] : m_trackers) {
            watcher(network::Interface {index, tracker.name()}, tracker.networkAddresses());
            tracker.clearFlag(DirtyFlag::NetworkAddressesChanged);
        }
    }
    return m_networkAddressNotifier.addWatcher(watcher);
}

void NetworkMonitor::removeNetworkAddressWatcher(const NetworkAddressWatcherToken& token)
{
    m_networkAddressNotifier.removeWatcher(token);
}

auto NetworkMonitor::addGatewayAddressWatcher(const GatewayAddressWatcher& watcher,
                                              const InitialSnapshotMode initialSnapshot) -> GatewayAddressWatcherToken
{
    if (initialSnapshot == InitialSnapshotMode::InitialSnapshot) {
        for (auto& [index, tracker] : m_trackers) {
            watcher(network::Interface {index, tracker.name()}, tracker.gatewayAddress());
            tracker.clearFlag(DirtyFlag::GatewayAddressChanged);
        }
    }
    return m_gatewayAddressNotifier.addWatcher(watcher);
}

void NetworkMonitor::removeGatewayAddressWatcher(const GatewayAddressWatcherToken& token)
{
    m_gatewayAddressNotifier.removeWatcher(token);
}

auto NetworkMonitor::addMacAddressWatcher(const MacAddressWatcher& watcher, const InitialSnapshotMode initialSnapshot)
    -> MacAddressWatcherToken
{
    if (initialSnapshot == InitialSnapshotMode::InitialSnapshot) {
        for (auto& [index, tracker] : m_trackers) {
            watcher(network::Interface {index, tracker.name()}, tracker.macAddress());
            tracker.clearFlag(DirtyFlag::MacAddressChanged);
        }
    }
    return m_macAddressNotifier.addWatcher(watcher);
}

void NetworkMonitor::removeMacAddressWatcher(const MacAddressWatcherToken& token)
{
    m_macAddressNotifier.removeWatcher(token);
}

auto NetworkMonitor::addBroadcastAddressWatcher(const BroadcastAddressWatcher& watcher,
                                                const InitialSnapshotMode initialSnapshot)
    -> BroadcastAddressWatcherToken
{
    if (initialSnapshot == InitialSnapshotMode::InitialSnapshot) {
        for (auto& [index, tracker] : m_trackers) {
            watcher(network::Interface {index, tracker.name()}, tracker.broadcastAddress());
            tracker.clearFlag(DirtyFlag::BroadcastAddressChanged);
        }
    }
    return m_broadcastAddressNotifier.addWatcher(watcher);
}

void NetworkMonitor::removeBroadcastAddressWatcher(const BroadcastAddressWatcherToken& token)
{
    m_broadcastAddressNotifier.removeWatcher(token);
}

auto NetworkMonitor::addEnumerationDoneWatcher(const EnumerationDoneWatcher& watcher)
    -> std::optional<EnumerationDoneWatcherToken>
{
    if (m_cacheState == CacheState::WaitingForChanges) {
        watcher();
        return std::nullopt;  // already done with enumeration, no need to notify later
    }
    return m_enumerationDoneNotifier.addWatcher(watcher);
}

void NetworkMonitor::removeEnumerationDoneWatcher(const EnumerationDoneWatcherToken& token)
{
    m_enumerationDoneNotifier.removeWatcher(token);
}

void NetworkMonitor::receiveAndProcess()
{
    if (!m_mnlSocket) {
        return;
    }
    spdlog::trace("Receiving messages from mnl socket");
    auto receiveResult = mnl_socket_recvfrom(m_mnlSocket.get(), m_receiveBuffer.data(), m_receiveBuffer.size());
    spdlog::trace("Received {} bytes", receiveResult);
    while (receiveResult > 0) {
        updateStats(receiveResult);
        if (m_runtimeOptions.test(RuntimeFlag::DumpPackets)) {
            dumpPacket(receiveResult);
        }
        const auto seqNo = isEnumerating() ? m_sequenceNumber : 0;
        const auto callbackResult = mnl_cb_run(m_receiveBuffer.data(),
                                               receiveResult,
                                               seqNo,
                                               m_portid,
                                               &NetworkMonitor::dispatchMnMessageCallbackToSelf,
                                               this);
        if (handleCallbackResult(callbackResult)) {
            break;
        }
        printStatsForNerdsIfEnabled();
        notifyChanges();
        if (m_mnlSocket) {
            receiveResult = mnl_socket_recvfrom(m_mnlSocket.get(), m_receiveBuffer.data(), m_receiveBuffer.size());
        }
    }
}

void NetworkMonitor::updateStats(const ssize_t receiveResult)
{
    m_stats.packetsReceived++;
    m_stats.bytesReceived += receiveResult;
}

void NetworkMonitor::dumpPacket(const ssize_t receiveResult) const
{
    std::ignore = fflush(stderr);
    std::ignore = fflush(stdout);
    mnl_nlmsg_fprintf(stdout, m_receiveBuffer.data(), receiveResult, 0);
}

auto NetworkMonitor::handleCallbackResult(const int callbackResult) -> bool
{
    if (callbackResult == MNL_CB_ERROR) {
        if (errno == EPROTO) {
            if (isEnumerating()) {
                spdlog::debug("Received EPROTO while enumerating, retrying");
                retryLastDumpRequest();
            } else {
                pwarn("mnl_cb_run");
            }
        }
        return true;
    }
    if (callbackResult == MNL_CB_STOP) {
        if (isEnumeratingLinks()) {
            m_cacheState = CacheState::EnumeratingAddresses;
            spdlog::debug("Requesting RTM_GETADDR");
            sendDumpRequest(RTM_GETADDR);
        } else if (isEnumeratingAddresses()) {
            m_cacheState = CacheState::EnumeratingRoutes;
            spdlog::debug("Requesting RTM_GETROUTE");
            sendDumpRequest(RTM_GETROUTE);
        } else if (isEnumeratingRoutes()) {
            m_cacheState = CacheState::WaitingForChanges;
            spdlog::debug("Done with enumeration of initial information");
            spdlog::debug("Tracking changes for {} interfaces", m_trackers.size());
            if (m_enumerationDoneNotifier.hasWatchers()) {
                m_enumerationDoneNotifier.notify();
            }
            printStatsForNerdsIfEnabled();
            return true;
        } else {
            if (m_mnlSocket) {
                pwarn("Unexpected MNL_CB_STOP");
            } else {
                return true;  // someone may call stop() while we are notifying watchers
            }
        }
    }
    return false;
}

void NetworkMonitor::sendDumpRequest(const uint16_t msgType)
{
    nlmsghdr* nlh = mnl_nlmsg_put_header(m_sendBuffer.data());
    nlh->nlmsg_type = msgType;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nlh->nlmsg_seq = ++m_sequenceNumber;
    auto* gen = static_cast<rtgenmsg*>(mnl_nlmsg_put_extra_header(nlh, sizeof(struct rtgenmsg)));
    gen->rtgen_family = AF_UNSPEC;
    mnl_attr_put_u32(nlh, IFLA_EXT_MASK, RTEXT_FILTER_SKIP_STATS);
    const auto ret = mnl_socket_sendto(m_mnlSocket.get(), nlh, nlh->nlmsg_len);
    if (ret < 0) {
        pfatal("mnl_socket_sendto");
    }
    m_stats.packetsSent++;
    m_stats.bytesSent += ret;
}

void NetworkMonitor::retryLastDumpRequest()
{
    spdlog::debug("Retrying last dump request with sequence number {}", m_sequenceNumber);
    static_assert(alignof(nlmsghdr) <= alignof(std::max_align_t), "nlmsghdr alignment requirements not met");
    const auto* buf = static_cast<const void*>(m_sendBuffer.data());
    const auto* nlh = static_cast<const nlmsghdr*>(buf);
    const auto ret = mnl_socket_sendto(m_mnlSocket.get(), nlh, nlh->nlmsg_len);
    if (ret < 0) {
        pfatal("mnl_socket_sendto");
    }
    m_stats.packetsSent++;
    m_stats.bytesSent += ret;
}

auto NetworkMonitor::mnlMessageCallback(const nlmsghdr* n) -> int
{
    if (!m_mnlSocket) {
        return MNL_CB_STOP;  // someone may call stop() while we are processing messages
    }
    m_stats.msgsReceived++;
    switch (const auto t = n->nlmsg_type) {
        case RTM_NEWLINK:
        case RTM_DELLINK: {
            const auto* ifi = static_cast<const ifinfomsg*>(mnl_nlmsg_get_payload(n));
            parseLinkMessage(n, ifi);
        } break;
        case RTM_NEWADDR:
        case RTM_DELADDR: {
            const auto* ifa = static_cast<const ifaddrmsg*>(mnl_nlmsg_get_payload(n));
            parseAddressMessage(n, ifa);
        } break;
        case RTM_NEWROUTE:
        case RTM_DELROUTE: {
            const auto* rt = static_cast<const rtmsg*>(mnl_nlmsg_get_payload(n));
            parseRouteMessage(n, rt);
        } break;
        default:
            spdlog::warn("ignoring unexpected message type: {}", t);
            break;
    }
    return MNL_CB_OK;
}

auto NetworkMonitor::ensureNameCurrent(const uint32_t ifIndex, const std::optional<std::string>& name)
    -> NetworkInterfaceStatusTracker&
{
    const auto before = m_trackers.size();
    auto& cacheEntry = m_trackers[ifIndex];

    // Sometimes interfaces are renamed, account for that
    if (name.has_value()) {
        cacheEntry.setName(name.value());
    }
    if (before != m_trackers.size() || cacheEntry.isDirty(DirtyFlag::NameChanged)) {
        notifyInterfacesSnapshot();
        cacheEntry.clearFlag(DirtyFlag::NameChanged);
    }
    return cacheEntry;
}

void NetworkMonitor::parseLinkMessage(const nlmsghdr* nlhdr, const ifinfomsg* ifi)
{
    spdlog::trace("Parsing link message for interface index {}", ifi->ifi_index);
    m_stats.linkMessagesSeen++;
    const auto attributes =
        Attributes::parse(nlhdr, sizeof(*ifi), IFLA_MAX, m_stats.seenAttributes, m_stats.unknownAttributes);
    const auto itfName = attributes.getString(IFLA_IFNAME);
    if (ifi->ifi_type != ARPHRD_ETHER && ifi->ifi_type != ARPHRD_IEEE80211) {
        if (!m_runtimeOptions.test(RuntimeFlag::IncludeNonIeee802)) {
            spdlog::debug("Discarding interface {}: {} (use --include_non_ieee802 to include)",
                          ifi->ifi_index,
                          itfName.value_or("unknown"));
            m_stats.msgsDiscarded++;
            return;
        }
        spdlog::trace("Including non-IEEE 802.X interface {}: {}", ifi->ifi_index, itfName.value_or("unknown"));
    }
    if (nlhdr->nlmsg_type == RTM_DELLINK) {
        spdlog::trace("removing interface with index {}", ifi->ifi_index);
        m_trackers.erase(ifi->ifi_index);
        notifyInterfacesSnapshot();
        return;
    }

    auto& cacheEntry = ensureNameCurrent(ifi->ifi_index, itfName);
    const NetworkInterfaceStatusTracker::LinkFlags linkFlags(ifi->ifi_flags);
    cacheEntry.updateLinkFlags(linkFlags);

    if (const auto operationalStateOpt = attributes.getU8(IFLA_OPERSTATE); operationalStateOpt.has_value()) {
        cacheEntry.setOperationalState(static_cast<OperationalState>(operationalStateOpt.value()));
    }

    if (const auto macAddressOpt = attributes.getEthernetAddress(IFLA_ADDRESS); macAddressOpt.has_value()) {
        cacheEntry.setMacAddress(macAddressOpt.value());
    } else {
        spdlog::warn("Interface {}: {} has no MAC address", ifi->ifi_index, cacheEntry.name());
    }

    if (const auto broadcastAddressOpt = attributes.getEthernetAddress(IFLA_BROADCAST); broadcastAddressOpt.has_value())
    {
        cacheEntry.setBroadcastAddress(broadcastAddressOpt.value());
    } else {
        spdlog::warn("Interface {}: {} has no broadcast address", ifi->ifi_index, cacheEntry.name());
    }
}

void NetworkMonitor::parseAddressMessage(const nlmsghdr* nlhdr, const ifaddrmsg* ifa)
{
    spdlog::trace("Parsing address message for interface index {}", ifa->ifa_index);
    m_stats.addressMessagesSeen++;
    if (!m_trackers.contains(ifa->ifa_index)) {
        m_stats.msgsDiscarded++;
        return;
    }
    if (m_runtimeOptions.test(RuntimeFlag::PreferredFamilyV4) && ifa->ifa_family != AF_INET) {
        m_stats.msgsDiscarded++;
        return;
    }

    if (m_runtimeOptions.test(RuntimeFlag::PreferredFamilyV6) && ifa->ifa_family != AF_INET6) {
        m_stats.msgsDiscarded++;
        return;
    }

    const auto attributes =
        Attributes::parse(nlhdr, sizeof(*ifa), IFA_MAX, m_stats.seenAttributes, m_stats.unknownAttributes);

    uint32_t flags = ifa->ifa_flags;  // will be overwritten if IFA_FLAGS is present
    ip::Address address;
    std::optional<ip::Address> broadcast = std::nullopt;  // will be overwritten if IFA_BROADCAST is present
    uint8_t prot = IFAPROT_UNSPEC;  // will be overwritten if IFA_PROTO is present

    auto& cacheEntry = ensureNameCurrent(ifa->ifa_index, attributes.getString(IFA_LABEL));
    if (const auto flagsOpt = attributes.getU32(IFA_FLAGS); flagsOpt.has_value()) {
        flags = flagsOpt.value();
    }
    if (const auto protoOpt = attributes.getU8(IFA_PROTO); protoOpt.has_value()) {
        prot = protoOpt.value();
    }
    if (const auto broadcastV4Opt = attributes.getIpV4Address(IFA_BROADCAST); broadcastV4Opt.has_value()) {
        broadcast = broadcastV4Opt.value();
    }
    if (const auto localV4Opt = attributes.getIpV4Address(IFA_LOCAL); localV4Opt.has_value()) {
        address = localV4Opt.value();
    }
    if (const auto addressV6Opt = attributes.getIpV6Address(IFA_ADDRESS); addressV6Opt.has_value()) {
        address = addressV6Opt.value();
    }
    const network::Address networkAddress {address,
                                           broadcast,
                                           ifa->ifa_prefixlen,
                                           network::fromRtnlScope(ifa->ifa_scope),
                                           network::AddressFlags(flags),
                                           static_cast<network::AddressAssignmentProtocol>(prot)};
    if (nlhdr->nlmsg_type == RTM_NEWADDR) {
        cacheEntry.addNetworkAddress(networkAddress);
    } else if (nlhdr->nlmsg_type == RTM_DELADDR) {
        cacheEntry.removeNetworkAddress(networkAddress);
    }
}

void NetworkMonitor::parseRouteMessage(const nlmsghdr* nlhdr, const rtmsg* rtm)
{
    spdlog::trace("Parsing route message");
    m_stats.routeMessagesSeen++;
    if (rtm->rtm_family != AF_INET) {
        m_stats.msgsDiscarded++;
        return;
    }
    if (m_runtimeOptions.test(RuntimeFlag::PreferredFamilyV6) && rtm->rtm_family != AF_INET6) {
        m_stats.msgsDiscarded++;
        return;
    }

    const auto attributes =
        Attributes::parse(nlhdr, sizeof(*rtm), RTA_MAX, m_stats.seenAttributes, m_stats.unknownAttributes);
    const auto ifIndexOpt = attributes.getU32(RTA_OIF);
    const auto gatewayV4Opt = attributes.getIpV4Address(RTA_GATEWAY);

    if (nlhdr->nlmsg_type == RTM_DELROUTE) {
        if (ifIndexOpt.has_value()) {
            if ((rtm->rtm_flags & RTNH_F_LINKDOWN) != 0U) {
                if (const auto itr = m_trackers.find(ifIndexOpt.value()); itr != m_trackers.end()) {
                    itr->second.clearGatewayAddress(GatewayClearReason::LinkDown);
                }
                return;
            }
            if (gatewayV4Opt.has_value()) {
                if (const auto itr = m_trackers.find(ifIndexOpt.value()); itr != m_trackers.end()) {
                    itr->second.clearGatewayAddress(GatewayClearReason::RouteDeleted);
                }
            }
        }
        return;
    }

    if (ifIndexOpt.has_value() && gatewayV4Opt.has_value()) {
        if (const auto itr = m_trackers.find(ifIndexOpt.value()); itr != m_trackers.end()) {
            itr->second.setGatewayAddress(gatewayV4Opt.value());
        }
    }
}

void NetworkMonitor::printStatsForNerdsIfEnabled()
{
    if (isEnumerating() || !m_runtimeOptions.test(RuntimeFlag::StatsForNerds)) {
        return;
    }
    spdlog::info("{:=^48}", "Stats for nerds");
    spdlog::info(
        "uptime    {}ms",
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_stats.startTime)
            .count());
    spdlog::info("sent      {} bytes in {} packets", m_stats.bytesSent, m_stats.packetsSent);
    spdlog::info("received  {} bytes in {} packets", m_stats.bytesReceived, m_stats.packetsReceived);
    spdlog::info("received  {} rtnl messages", m_stats.msgsReceived);
    spdlog::info("discarded {} rtnl messages", m_stats.msgsDiscarded);
    spdlog::info("* seen");
    spdlog::info("          {} attribute entries", m_stats.seenAttributes);
    spdlog::info("          {} attributes unknown", m_stats.unknownAttributes);
    spdlog::info("          {} link messages", m_stats.linkMessagesSeen);
    spdlog::info("          {} address messages", m_stats.addressMessagesSeen);
    spdlog::info("          {} route messages", m_stats.routeMessagesSeen);

    spdlog::info("{:=^48}", "Interface details in cache");
    for (const auto& tracker : m_trackers | std::views::values) {
        spdlog::info(tracker);
        tracker.logNerdstats();
    }
    spdlog::info("{:=^48}", "=");
}

auto NetworkMonitor::dispatchMnMessageCallbackToSelf(const nlmsghdr* n, void* self) -> int
{
    return static_cast<NetworkMonitor*>(self)->mnlMessageCallback(n);
}

void NetworkMonitor::notifyChanges()
{
    for (auto& [index, tracker] : m_trackers) {
        spdlog::trace("checking {} for changes", tracker);
        if (m_operationalStateNotifier.hasWatchers() && tracker.isDirty(DirtyFlag::OperationalStateChanged)) {
            m_operationalStateNotifier.notify(network::Interface {index, tracker.name()}, tracker.operationalState());
            tracker.clearFlag(DirtyFlag::OperationalStateChanged);
        }
        if (m_networkAddressNotifier.hasWatchers() && tracker.isDirty(DirtyFlag::NetworkAddressesChanged)) {
            m_networkAddressNotifier.notify(network::Interface {index, tracker.name()}, tracker.networkAddresses());
            tracker.clearFlag(DirtyFlag::NetworkAddressesChanged);
        }
        if (m_gatewayAddressNotifier.hasWatchers() && tracker.isDirty(DirtyFlag::GatewayAddressChanged)) {
            m_gatewayAddressNotifier.notify(network::Interface {index, tracker.name()}, tracker.gatewayAddress());
            tracker.clearFlag(DirtyFlag::GatewayAddressChanged);
        }
        if (m_macAddressNotifier.hasWatchers() && tracker.isDirty(DirtyFlag::MacAddressChanged)) {
            m_macAddressNotifier.notify(network::Interface {index, tracker.name()}, tracker.macAddress());
            tracker.clearFlag(DirtyFlag::MacAddressChanged);
        }
        if (m_broadcastAddressNotifier.hasWatchers() && tracker.isDirty(DirtyFlag::BroadcastAddressChanged)) {
            m_broadcastAddressNotifier.notify(network::Interface {index, tracker.name()}, tracker.broadcastAddress());
            tracker.clearFlag(DirtyFlag::BroadcastAddressChanged);
        }
        if (m_linkFlagsNotifier.hasWatchers() && tracker.isDirty(DirtyFlag::LinkFlagsChanged)) {
            m_linkFlagsNotifier.notify(network::Interface {index, tracker.name()}, tracker.linkFlags());
            tracker.clearFlag(DirtyFlag::LinkFlagsChanged);
        }
    }
}

void NetworkMonitor::notifyInterfacesSnapshot()
{
    if (m_interfacesNotifier.hasWatchers()) {
        m_interfacesNotifier.notify(getInterfacesSnapshot());
    }
}

auto NetworkMonitor::getInterfacesSnapshot() const -> Interfaces
{
    Interfaces intfs;
    for (const auto& [idx, tracker] : m_trackers) {
        if (tracker.hasName()) {
            intfs.emplace(idx, tracker.name());
        }
    }
    return intfs;
}

}  // namespace monkas::monitor
