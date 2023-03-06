#include <gflags/gflags_declare.h>
#include <iostream>

#include "monitor/RtnlNetworkMonitor.h"

#include <gflags/gflags.h>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>

DECLARE_bool(nerdstats);
DEFINE_bool(nerdstats, false, "Enable stats for nerds");

DECLARE_uint32(family);
DEFINE_uint32(family, 0, "Preferred address family <4|6>");

// TODO: Use qt event loop with QSocketNotifier for async rtnl
// TODO: Implement some kind of IPC with pub/sub

int main(int argc, char *argv[])
{
    gflags::SetUsageMessage("<flags>\n");
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    spdlog::set_level(spdlog::level::trace);
    spdlog::flush_every(std::chrono::seconds(5));
    monkas::RuntimeOptions options{};
    if (FLAGS_nerdstats)
    {
        options |= monkas::RuntimeFlag::StatsForNerds;
    }
    if (FLAGS_family == 4)
    {
        options |= monkas::RuntimeFlag::PreferredFamilyV4;
    }
    if (FLAGS_family == 6)
    {
        options |= monkas::RuntimeFlag::PreferredFamilyV6;
    }

    monkas::RtnlNetworkMonitor mon(options);

    return mon.run();
}
