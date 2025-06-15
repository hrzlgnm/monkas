#include <gflags/gflags.h>
#include <monitor/NetworkInterfaceStatusTracker.hpp>
#include <monitor/RtnlNetworkMonitor.hpp>
#include <network/NetworkAddress.hpp>
#include <spdlog/spdlog.h>

DEFINE_bool(nerdstats, false, "Enable stats for nerds");

DEFINE_bool(dumppackets, false, "Enable dumping of rtnl packets");

DEFINE_uint32(family, 0, "Preferred address family <4|6>");

DEFINE_string(log_level, "info", "Set log level: trace, debug, info, warn, err, critical, off");

using namespace monkas;

/**
 * @brief Entry point for the rtnetlink network monitor CLI application.
 *
 * Parses command-line flags to configure logging level, monitoring options, and preferred IP family, then initializes
 * and runs the network monitor. Returns the monitor's exit code.
 *
 * @return int Exit code from the network monitor.
 */

int main(int argc, char *argv[])
{
    gflags::SetUsageMessage("<flags>\n");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    std::transform(FLAGS_log_level.begin(), FLAGS_log_level.end(), FLAGS_log_level.begin(), ::tolower);
    if (auto level = spdlog::level::from_str(FLAGS_log_level); level == spdlog::level::off && FLAGS_log_level != "off")
    {
        SPDLOG_ERROR("invalid log level '{}', using 'info' instead", FLAGS_log_level);
        spdlog::set_level(spdlog::level::info);
    }
    else
    {
        spdlog::set_level(level);
    }
    spdlog::flush_every(std::chrono::seconds(5));
    RuntimeOptions options{};
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
    RtnlNetworkMonitor mon(options);
    std::ignore = mon.addInterfacesListener(
        [](const Interfaces &interfaces) { spdlog::info("Interfaces changed to: {}", fmt::join(interfaces, ", ")); });
    std::ignore = mon.addOperationalStateListener([](const network::Interface &iface, OperationalState state) {
        spdlog::info("{} changed operational state to {}", iface, state);
    });
    std::ignore = mon.addNetworkAddressListener([](const network::Interface &iface, const NetworkAddresses &addresses) {
        spdlog::info("{} changed addresses to {}", iface, fmt::join(addresses, ", "));
    });

    return mon.run();
}
