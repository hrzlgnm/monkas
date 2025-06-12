#include <gflags/gflags.h>
#include <gflags/gflags_declare.h>
#include <monitor/RtnlNetworkMonitor.hpp>
#include <network/NetworkAddress.hpp>
#include <spdlog/spdlog.h>

DECLARE_bool(nerdstats);
DEFINE_bool(nerdstats, false, "Enable stats for nerds");

DECLARE_bool(dumppackets);
DEFINE_bool(dumppackets, false, "Enable dumping of rtnl packets");

DECLARE_uint32(family);
DEFINE_uint32(family, 0, "Preferred address family <4|6>");

DECLARE_string(log_level);
DEFINE_string(log_level, "info", "Set log level: trace, debug, info, warn, err, critical, off");

// TODO: Use qt event loop with QSocketNotifier for async rtnl

int main(int argc, char *argv[])
{
    gflags::SetUsageMessage("<flags>\n");
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    spdlog::level::level_enum level = spdlog::level::info;
    if (FLAGS_log_level == "trace")
        level = spdlog::level::trace;
    else if (FLAGS_log_level == "debug")
        level = spdlog::level::debug;
    else if (FLAGS_log_level == "info")
        level = spdlog::level::info;
    else if (FLAGS_log_level == "warn")
        level = spdlog::level::warn;
    else if (FLAGS_log_level == "err")
        level = spdlog::level::err;
    else if (FLAGS_log_level == "critical")
        level = spdlog::level::critical;
    else if (FLAGS_log_level == "off")
        level = spdlog::level::off;

    spdlog::set_level(level);
    spdlog::flush_every(std::chrono::seconds(5));
    monkas::RuntimeOptions options{};
    if (FLAGS_nerdstats)
    {
        options |= monkas::RuntimeFlag::StatsForNerds;
    }
    if (FLAGS_dumppackets)
    {
        options |= monkas::RuntimeFlag::DumpPackets;
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
