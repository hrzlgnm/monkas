#include <doctest/doctest.h>
#include <ethernet/Address.hpp>
#include <ip/Address.hpp>
#include <monitor/NetworkInterfaceStatusTracker.hpp>

namespace
{

// NOLINTBEGIN(*)
using namespace monkas::monitor;
using namespace monkas;

TEST_SUITE("[monitor::NetworkInterfaceStatusTracker]")
{
    NetworkInterfaceStatusTracker tracker;

    TEST_CASE("NetworkInterfaceStatusTracker default state")
    {
        CHECK(tracker.hasName() == false);
        CHECK(tracker.name() == "");
        CHECK(tracker.isDirty() == false);
    }

    TEST_CASE("NetworkInterfaceStatusTracker set and check name")
    {
        tracker.setName("eth0");
        CHECK(tracker.hasName());
        CHECK(tracker.name() == "eth0");
        CHECK(tracker.isDirty());
        CHECK(tracker.isDirty(DirtyFlag::NameChanged));
    }

    TEST_CASE("NetworkInterfaceStatusTracker operational state")
    {
        tracker.setOperationalState(NetworkInterfaceStatusTracker::OperationalState::Up);
        CHECK(tracker.operationalState() == NetworkInterfaceStatusTracker::OperationalState::Up);
        CHECK(tracker.isDirty(DirtyFlag::OperationalStateChanged));
    }

    TEST_CASE("NetworkInterfaceStatusTracker MAC address")
    {
        ethernet::Address addr;
        tracker.setMacAddress(addr);
        CHECK(tracker.macAddress() == addr);
        CHECK(tracker.isDirty(DirtyFlag::MacAddressChanged));
    }

    TEST_CASE("NetworkInterfaceStatusTracker broadcast address")
    {
        ethernet::Address brd;
        tracker.setBroadcastAddress(brd);
        CHECK(tracker.broadcastAddress() == brd);
        CHECK(tracker.isDirty(DirtyFlag::BroadcastAddressChanged));
    }

    TEST_CASE("NetworkInterfaceStatusTracker clearFlag")
    {
        tracker.setName("eth0");
        CHECK(tracker.isDirty(DirtyFlag::NameChanged));
        tracker.clearFlag(DirtyFlag::NameChanged);
        CHECK_FALSE(tracker.isDirty(DirtyFlag::NameChanged));
    }

    TEST_CASE("NetworkInterfaceStatusTracker age")
    {
        tracker.setName("eth0");
        auto age = tracker.age();
        CHECK(age.count() >= 0);  // Ensure age is non-negative
    }
}

// NOLINTEND(*)
}  // namespace
