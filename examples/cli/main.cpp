#include <fmt/std.h>
#include <gflags/gflags.h>
#include <monitor/NetworkInterfaceStatusTracker.hpp>
#include <monitor/NetworkMonitor.hpp>
#include <network/Address.hpp>
#include <network/Interface.hpp>
#include <spdlog/spdlog.h>

DEFINE_bool(nerdstats, false, "Enable stats for nerds");

DEFINE_bool(dumppackets, false, "Enable dumping of rtnl packets");

DEFINE_bool(exit_after_enumeration, false, "Exit after enumeration is done");

DEFINE_bool(include_non_ieee802, false, "Include non IEEE 802.X interfaces in the enumeration");

DEFINE_uint32(family, 0, "Preferred address family <0|4|6>");
DEFINE_validator(family,
                 [](const char* /*flagname*/, const uint32_t value) { return value == 0 || value == 4 || value == 6; });

DEFINE_string(log_level, "info", "Set log level: trace, debug, info, warn, err, critical, off");

// NOLINTNEXTLINE(google-build-*)
using namespace monkas::monitor;
// NOLINTNEXTLINE(google-build-*)
using namespace monkas::network;
// NOLINTNEXTLINE(google-build-*)
using namespace monkas;

/**
 * @brief Runs the rtnetlink network monitor CLI application.
 *
 * Parses command-line flags to configure logging, monitoring options, and IP family preferences, then initializes and
 * runs the network monitor event loop. Registers watchers for interface and address changes, and optionally exits after
 * initial enumeration.
 *
 * @return int Always returns EXIT_SUCCESS upon completion.
 */

auto main(int argc, char* argv[]) -> int
{
    gflags::SetUsageMessage("<flags>\n");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    std::ranges::transform(FLAGS_log_level, FLAGS_log_level.begin(), ::tolower);
    if (const auto level = spdlog::level::from_str(FLAGS_log_level);
        level == spdlog::level::off && FLAGS_log_level != "off")
    {
        SPDLOG_ERROR("invalid log level '{}', using 'info' instead", FLAGS_log_level);
        spdlog::set_level(spdlog::level::info);
    } else {
        spdlog::set_level(level);
    }
    constexpr auto FLUSH_EVERY = std::chrono::seconds(5);
    spdlog::flush_every(FLUSH_EVERY);
    RuntimeOptions options;
    if (FLAGS_nerdstats) {
        options.set(RuntimeFlag::StatsForNerds);
    }
    if (FLAGS_dumppackets) {
        options.set(RuntimeFlag::DumpPackets);
    }
    if (FLAGS_family == 4) {
        options.set(RuntimeFlag::PreferredFamilyV4);
    }
    constexpr auto V6 = 6U;
    if (FLAGS_family == V6) {
        options.set(RuntimeFlag::PreferredFamilyV6);
    }
    if (FLAGS_include_non_ieee802) {
        options.set(RuntimeFlag::IncludeNonIeee802);
    }
    NetworkMonitor mon(options);
    mon.enumerateInterfaces();
    std::ignore = mon.addInterfacesWatcher([](const Interfaces& interfaces)
                                           { spdlog::info("Interfaces changed to: {}", fmt::join(interfaces, ", ")); },
                                           InitialSnapshotMode::InitialSnapshot);
    std::ignore = mon.addOperationalStateWatcher([](const Interface& iface, OperationalState state)
                                                 { spdlog::info("{} changed operational state to {}", iface, state); },
                                                 InitialSnapshotMode::InitialSnapshot);
    std::ignore =
        mon.addNetworkAddressWatcher([](const Interface& iface, const Addresses& addresses)
                                     { spdlog::info("{} changed addresses to {}", iface, fmt::join(addresses, ", ")); },
                                     InitialSnapshotMode::InitialSnapshot);
    std::ignore = mon.addGatewayAddressWatcher(
        [](const Interface& iface, const std::optional<ip::Address>& gateway)
        {
            spdlog::info("{} changed gateway address to {}",
                         iface,
                         gateway.transform([](const auto& a) { return a.toString(); }).value_or("None"));
        },
        InitialSnapshotMode::InitialSnapshot);
    std::ignore = mon.addMacAddressWatcher([](const Interface& iface, const ethernet::Address& mac)
                                           { spdlog::info("{} changed MAC address to {}", iface, mac); },
                                           InitialSnapshotMode::InitialSnapshot);
    std::ignore =
        mon.addBroadcastAddressWatcher([](const Interface& iface, const ethernet::Address& broadcast)
                                       { spdlog::info("{} changed broadcast address to {}", iface, broadcast); },
                                       InitialSnapshotMode::InitialSnapshot);
    std::ignore = mon.addEnumerationDoneWatcher(
        [&mon]()
        {
            spdlog::info("Enumeration done");
            if (FLAGS_exit_after_enumeration) {
                spdlog::info("Exiting after enumeration is done");
                mon.stop();
            }
        });

    mon.run();
    return EXIT_SUCCESS;
}
