#ifndef NETLINKMONITORNML_H
#define NETLINKMONITORNML_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <unordered_set>
#include <chrono>
#include <libmnl/libmnl.h>

// TODO: move to own translation unit
// TODO: camelCaes
// TODO: address struct with family, scope, prefix lenght
// TODO: own member for mac address
// TODO: check whether changing mac will remove the hwaddress and add a new one
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


// TODO: more expressive name, something with link and address
class NetlinkMonitorNlm
{
public:
    NetlinkMonitorNlm();
    int run();

private:
    void startReceiving();
    void requestInfo(uint16_t t);


    void handleLinkMessage(const struct ifinfomsg *ifi, bool n);
    void handleAddrMessage(const struct ifaddrmsg *ifa, bool n);
    void parseAttributes(const struct nlmsghdr *n, size_t offset);
    void ensureInterfaceNamed(int interface_index);
    void dumpState();

    static int dipatchMnlDataCallbackToSelf(const struct nlmsghdr *n, void *self);
    int mnlMessageCallback(const struct nlmsghdr *n);
    static int mnlAttributeCallback(const struct nlattr *a, void *self);
    int mnlAttributeCallback(const nlattr *a);

private:
    std::vector<const nlattr*> m_lastSeenAttributes;
    std::vector<char> m_buffer;
    std::unique_ptr<mnl_socket, int(*)(mnl_socket*)> m_mnlSocket;
    std::map<int, NetworkInterfaceDescriptor> m_cache;

    enum class CacheState {
        FillLinkInfo,
        FillAddressInfo,
        WaitForChanges
    } m_cacheState{CacheState::FillLinkInfo};

    unsigned m_portid{};
    unsigned m_sequenceCounter{};

    struct stats {
        uint64_t bytesSent{};
        uint64_t bytesReceived{};
        uint64_t packetsSent{};
        uint64_t packetsReceived{};
        uint64_t msgsReceived{};
        uint64_t parsedAttributes{};
    } m_stats;

};
#endif
