#ifndef MONKA_H
#define MONKA_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <unordered_set>

#include <libmnl/libmnl.h>


class monka_steer {
public:
    monka_steer();
    const std::string &name() const;
    void set_name(const std::string & name);

    uint16_t operstate() const;
    void set_operstate(uint16_t operstate);

    void push_addr(const std::string& addr);
    void pull_addr(const std::string& addr);
private:
    friend std::ostream& operator<<(std::ostream& o, const monka_steer& s);
    std::string maname;
    uint16_t maoperstate;
    std::unordered_set<std::string> maaddrs;
};



class monkaS
{
public:
    monkaS();
    int run();

private:
    void yoink();
    void request_info(uint16_t t);

    int monka_data_callback(const struct nlmsghdr *n);
    int monka_attribute_callback(const nlattr *a);
    void monka_link(const struct ifinfomsg *ifi, bool n);
    void monka_addr(const struct ifaddrmsg *ifa, bool n);
    void monka_flags(const struct nlmsghdr *n, size_t len);
    void monka_dump_state();

    void ensure_interface_is_named(int interface_index);

    static int mnl_data_callback(const struct nlmsghdr *n, void *self);
    static int mnl_attribute_cb(const struct nlattr *a, void *self);

private:
    std::vector<const nlattr*> matb;
    std::vector<char> mabuf;
    std::unique_ptr<mnl_socket, int(*)(mnl_socket*)> manl;
    unsigned maportid;
    unsigned maseq;
    std::map<int, monka_steer> mach;
    enum class monka_state {
        fetch_links,
        fetch_addresses,
        listen_for_changes
    } maca{monka_state::fetch_links};
};

#endif // MONKA_H
