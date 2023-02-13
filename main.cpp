#include <gflags/gflags_declare.h>
#include <iostream>

#include "monitor/RtnlNetworkMonitor.h"

#include <gflags/gflags.h>
#include <glog/logging.h>

DECLARE_bool(nerdstats);
DEFINE_bool(nerdstats, false, "Enable stats for nerds");

DECLARE_uint32(family);
DEFINE_uint32(family, 0, "Preferred address family (4|6)");

// TODO: Use qt event loop with QSocketNotifier for async netlink reading
// TODO: Implement some kind of IPC
int main(int argc, char *argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    monkas::Options options{};
    if (FLAGS_nerdstats)
    {
        options |= monkas::OptFlag::StatsForNerds;
    }
    if (FLAGS_family == 4)
    {
        options |= monkas::OptFlag::PreferredFamilyV4;
    }
    if (FLAGS_family == 6)
    {
        options |= monkas::OptFlag::PreferredFamilyV6;
    }

    monkas::RtnlNetworkMonitor mon(options);

    return mon.run();
}
