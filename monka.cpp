#include "monka.h"
#include <arpa/inet.h>
#include <iostream>
#include <linux/if_link.h>
#include <linux/rtnetlink.h>
#include <memory.h>
#include <net/if.h>
#include <thread>

monkaS::monkaS()
    : manl{mnl_socket_open(NETLINK_ROUTE), mnl_socket_close},
      maportid{mnl_socket_get_portid(manl.get())}, maseq{} {
    mabuf.resize(MNL_SOCKET_BUFFER_SIZE);
    matb.resize(IFLA_MAX + 1);
    unsigned groups = RTMGRP_LINK;
    groups |= RTMGRP_IPV4_IFADDR;
    groups |= RTMGRP_IPV6_IFADDR;

    if (mnl_socket_bind(manl.get(), groups, MNL_SOCKET_AUTOPID) < 0) {
        perror("mnl_socket_bind");
        exit(EXIT_FAILURE);
    }
}

int monkaS::run() {
    puts("yoinking link infos\n");
    request_info(RTM_GETLINK);
    yoink();
    return 0;
}

void monkaS::yoink() {
    auto ret = mnl_socket_recvfrom(manl.get(), &mabuf[0], mabuf.size());
    while (ret > 0) {
        // mnl_nlmsg_fprintf(stdout, &mabuf[0], ret, 0);
        ret = mnl_cb_run(&mabuf[0], ret, maseq, maportid,
                &monkaS::mnl_data_callback, this);
        if (ret < MNL_CB_STOP) {
            printf("bork\n");
            break;
        }
        if (ret == MNL_CB_STOP) {
            if (maca == monka_state::fetch_links) {
                maca = monka_state::fetch_addresses;
                puts("yoinking address infos\n");
                request_info(RTM_GETADDR);
            } else if (maca == monka_state::fetch_addresses) {
                maca = monka_state::listen_for_changes;
                maseq = 0;
                printf("initial information yoinked successfully\n");
                printf("listening for changes of: %zu interfaces\n", mach.size());
                monka_dump_state();
                fflush(stdout);
            }
        }
        ret = mnl_socket_recvfrom(manl.get(), &mabuf[0], mabuf.size());
    }
}

void monkaS::request_info(uint16_t t) {
    nlmsghdr *nlh = mnl_nlmsg_put_header(&mabuf[0]);
    nlh->nlmsg_type = t;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nlh->nlmsg_seq = maseq = time(NULL);
    rtgenmsg *rt =
            (rtgenmsg *)mnl_nlmsg_put_extra_header(nlh, sizeof(struct rtgenmsg));
    rt->rtgen_family = AF_PACKET;
    if (mnl_socket_sendto(manl.get(), nlh, nlh->nlmsg_len) < 0) {
        perror("mnl_socket_sendto");
        exit(EXIT_FAILURE);
    }
}

int monkaS::monka_data_callback(const nlmsghdr *n) {
    const auto t = n->nlmsg_type;
    switch (t) {
    case RTM_NEWLINK:
    case RTM_DELLINK: {
        ifinfomsg *ifi = (ifinfomsg *)mnl_nlmsg_get_payload(n);
        monka_flags(n, sizeof(*ifi));
        monka_link(ifi, t == RTM_NEWLINK);
    }
    case RTM_NEWADDR:
    case RTM_DELADDR: {
        ifaddrmsg *ifa = (ifaddrmsg *)mnl_nlmsg_get_payload(n);
        monka_flags(n, sizeof(*ifa));
        monka_addr(ifa, t == RTM_NEWADDR);
    }
    }
    fflush(stdout);
    return MNL_CB_OK;
}

int monkaS::monka_attribute_callback(const nlattr *a) {
    if (mnl_attr_type_valid(a, IFLA_MAX) >= 0) {
        auto type = mnl_attr_get_type(a);
        matb[type] = a;
    }
    return MNL_CB_OK;
}

void monkaS::ensure_interface_is_named(int interface_index) {
    auto &ppHot = mach[interface_index];
    if (!ppHot.name().empty()) {
        return;
    }
    if (matb[IFLA_IFNAME]) {
        ppHot.set_name(mnl_attr_get_str(matb[IFLA_IFNAME]));
    } else {
        std::vector<char> namebuf(IF_NAMESIZE, '\0');
        auto name = if_indextoname(interface_index, &namebuf[0]);
        if (name) {
            ppHot.set_name(name);
        } else {
            std::cerr << "failed to determine interface name from index "
                      << interface_index << "\n";
        }
    }
}

void monkaS::monka_link(const ifinfomsg *ifi, bool n) {
    ensure_interface_is_named(ifi->ifi_index);
    mach[ifi->ifi_index].set_operstate(mnl_attr_get_u16(matb[IFLA_OPERSTATE]));
    monka_dump_state();
}

void monkaS::monka_addr(const ifaddrmsg *ifa, bool n) {
    ensure_interface_is_named(ifa->ifa_index);
    if (matb[IFLA_ADDRESS]) {
        void *addr = mnl_attr_get_payload(matb[IFA_ADDRESS]);
        char out[INET6_ADDRSTRLEN];
        if (inet_ntop(ifa->ifa_family, addr, out, sizeof(out))) {
            if (n) {
                mach[ifa->ifa_index].push_addr(out);
            } else {
                mach[ifa->ifa_index].pull_addr(out);
            }
        }
    }
    monka_dump_state();
}

void monkaS::monka_flags(const nlmsghdr *n, size_t offset) {
    std::fill(matb.begin(), matb.end(), nullptr);
    mnl_attr_parse(n, offset, &monkaS::mnl_attribute_cb, this);
}

void monkaS::monka_dump_state() {
    if (maca != monka_state::listen_for_changes)
        return;
    std::cout << "state: \n";
    for (const auto &c : mach) {
        std::cout << c.first << ":" << c.second << "\n";
    }
}

int monkaS::mnl_data_callback(const nlmsghdr *n, void *self) {
    return reinterpret_cast<monkaS *>(self)->monka_data_callback(n);
}

int monkaS::mnl_attribute_cb(const nlattr *a, void *self) {
    return reinterpret_cast<monkaS *>(self)->monka_attribute_callback(a);
}

monka_steer::monka_steer() : maoperstate{} {}

const std::string &monka_steer::name() const { return maname; }

void monka_steer::set_name(const std::string &name) {
    if (maname != name) {
        maname = name;
        std::cout << "name changed to: " << name << "\n";
    }
}

uint16_t monka_steer::operstate() const { return maoperstate; }

void monka_steer::set_operstate(uint16_t operstate) {
    if (maoperstate != operstate) {
        maoperstate = operstate;
        std::cout << maname << " operstate changed to: " << operstate << "\n";
    }
}

void monka_steer::push_addr(const std::string &addr) {
    auto res = maaddrs.emplace(addr);
    if (res.second) {
        std::cout << maname << " addr added: " << addr << "\n";
    }
}

void monka_steer::pull_addr(const std::string &addr) {
    auto res = maaddrs.erase(addr);
    if (res > 0) {
        std::cout << maname << " addr removed: " << addr << "\n";
    }
}

std::ostream &operator<<(std::ostream &o, const monka_steer &s) {
    o << s.name() << ":" << s.maoperstate << ":" << s.maaddrs.size();
    for (const auto &a : s.maaddrs) {
        o << " " << a;
    }
    return o;
}
