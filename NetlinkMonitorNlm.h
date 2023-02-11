#ifndef MONKA_H
#define MONKA_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <unordered_set>
#include <chrono>
#include <libmnl/libmnl.h>

using age_duration = std::chrono::duration<int64_t, std::milli>;
class NetworkInterfaceDescriptor {
public:
    enum class OperState : uint8_t {
        Unknown,
        NotPresent,
        Down,
        LowerLayerDown,
        Testing,
        Dormant,
        Up,
    };

    NetworkInterfaceDescriptor();
    bool has_name() const;
    const std::string &name() const;
    void set_name(const std::string & name);
    OperState operstate() const;
    void set_operstate(OperState operstate);
    void push_addr(const std::string& addr);
    void pull_addr(const std::string& addr);
    age_duration age() const;

private:
    friend std::ostream& operator<<(std::ostream& o, const NetworkInterfaceDescriptor& s);
    std::string m_name;
    OperState m_operstate{OperState::Unknown};
    std::unordered_set<std::string> m_addresses;
    std::chrono::time_point<std::chrono::steady_clock> m_lastChanged;
};

class NetlinkMonitorNlm
{
public:
    NetlinkMonitorNlm();
    int run();

private:
    void startReceiving();
    void requestInfo(uint16_t t);

    int mnlMessageCallback(const struct nlmsghdr *n);
    int mnlAttributeCallbackOnSelf(const nlattr *a);

    void handleLinkMessage(const struct ifinfomsg *ifi, bool n);
    void handleAddrMessage(const struct ifaddrmsg *ifa, bool n);
    void parseFlags(const struct nlmsghdr *n, size_t len);

    void dumpState();

    void ensureInterfaceNamed(int interface_index);

    static int runMnlDataCallbackOnSelf(const struct nlmsghdr *n, void *self);
    static int mnlAttributeCallbackOnSelf(const struct nlattr *a, void *self);

private:
    std::vector<const nlattr*> m_lastSeenAttributeTable;
    std::vector<char> m_buffer;
    std::unique_ptr<mnl_socket, int(*)(mnl_socket*)> m_mnlSocket;
    std::map<int, NetworkInterfaceDescriptor> m_cache;

    enum class CacheState {
        FetchLinkInfo,
        FetchAddressInfo,
        WaitForChanges
    } m_cacheState{CacheState::FetchLinkInfo};

    unsigned m_portid{};
    unsigned m_sequenceCounter{};

    struct stats {
        uint64_t nBytesSent{};
        uint64_t nBytesReceived{};
        uint64_t nPacketsSent{};
        uint64_t nPacketsReceived{};
        uint64_t nMsgs{};
        uint64_t nParsedAttrs{};
    } m_stats;

};

#endif // MONKA_H
