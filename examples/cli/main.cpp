#include <chrono>
#include <thread>

#include <fmt/std.h>
#include <fmt/ranges.h>
#include <gflags/gflags.h>
#include <monitor/NetworkInterfaceStatusTracker.hpp>
#include <monitor/NetworkMonitor.hpp>
#include <network/Address.hpp>
#include <network/Interface.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <unistd.h>

DEFINE_bool(nerdstats, false, "Enable stats for nerds");

DEFINE_bool(dumppackets, false, "Enable dumping of rtnl packets");

DEFINE_bool(exit_after_enumeration, false, "Exit after enumeration is done");

DEFINE_bool(include_non_ieee802, false, "Include non IEEE 802.X interfaces in the enumeration");
DEFINE_bool(log_to_file, false, "Enable logging to file");

DEFINE_uint32(family, 0, "Preferred address family <0|4|6>");
DEFINE_validator(family,
                 [](const char* /*flagname*/, const uint32_t value) -> bool
                 { return value == 0 || value == 4 || value == 6; });

DEFINE_string(log_level, "info", "Set log level: trace, debug, info, warn, err, critical, off");

DEFINE_uint32(enum_loop, 1, "Run enumeration loop N times, 0 means infinite");
DEFINE_uint64(loop_delay_us, 100, "Delay between enumeration loops in µs, at least 50");
DEFINE_validator(loop_delay_us, [](const char* /*flagname*/, const uint64_t value) -> bool { return value >= 50; });

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

    if (FLAGS_log_to_file) {
        const auto logFileName = fmt::format("/tmp/monka-{}.log", getpid());
        // Let the user know where to find the log file before switching to file logging
        spdlog::info("Logging to {}", logFileName);
        auto logger = spdlog::basic_logger_mt("monka", logFileName, true);
        logger->flush_on(spdlog::level::critical);
        logger->set_level(spdlog::get_level());
        spdlog::set_default_logger(logger);
    }

    constexpr auto FLUSH_EVERY = std::chrono::seconds(10);
    spdlog::flush_every(FLUSH_EVERY);
    RuntimeFlags options;
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

    if (FLAGS_enum_loop > 1 || FLAGS_enum_loop == 0) {
        auto loop = FLAGS_enum_loop;
        if (FLAGS_enum_loop == 0) {
            spdlog::info("Running enumeration loop infinitely with loop delay of {}µs", FLAGS_loop_delay_us);
        } else {
            spdlog::info(
                "Running enumeration loop {} times with loop delay of {}µs", FLAGS_enum_loop, FLAGS_loop_delay_us);
        }
        while (FLAGS_enum_loop == 0 || loop > 1) {
            NetworkMonitor mon(options);
            std::ignore = mon.enumerateInterfaces();
            loop--;
            std::this_thread::sleep_for(std::chrono::microseconds(FLAGS_loop_delay_us));
        }
    }

    NetworkMonitor mon(options);

    const auto intfs = mon.enumerateInterfaces();
    spdlog::info("Found {} interfaces: {}", intfs.size(), fmt::join(intfs, ", "));

    struct Sub final : monitor::Subscriber
    {
        void onInterfaceAdded(const Interface& iface) override { spdlog::info("Interface added: {}", iface); }

        void onInterfaceRemoved(const Interface& iface) override { spdlog::info("Interface removed: {}", iface); }

        void onInterfaceNameChanged(const Interface& iface) override
        {
            spdlog::info("{} changed name to {}", iface, iface.name());
        }

        void onLinkFlagsChanged(const Interface& iface, const LinkFlags& flags) override
        {
            spdlog::info("{} changed link flags to {}", iface, flags);
        }

        void onOperationalStateChanged(const Interface& iface, OperationalState state) override
        {
            spdlog::info("{} changed operational state to {}", iface, state);
        }

        void onNetworkAddressesChanged(const Interface& iface, const Addresses& addresses) override
        {
            spdlog::info("{} changed addresses to {}", iface, fmt::join(addresses, ", "));
        }

        void onGatewayAddressChanged(const Interface& iface, const std::optional<ip::Address>& gateway) override
        {
            spdlog::info("{} changed gateway address to {}",
                         iface,
                         gateway.transform([](const auto& a) { return a.toString(); }).value_or("None"));
        }

        void onMacAddressChanged(const Interface& iface, const ethernet::Address& mac) override
        {
            spdlog::info("{} changed MAC address to {}", iface, mac);
        }

        void onBroadcastAddressChanged(const Interface& iface, const ethernet::Address& broadcast) override
        {
            spdlog::info("{} changed broadcast address to {}", iface, broadcast);
        }
    };

    auto sub = std::make_shared<Sub>();
    mon.subscribe(intfs, sub);
    if (FLAGS_exit_after_enumeration) {
        spdlog::info("Exiting after enumeration is done");
        mon.stop();
    }

    mon.run();
    return EXIT_SUCCESS;
}
