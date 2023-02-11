#include <iostream>

#include "RtNetlinkEthernetLinkAndIpAddressMonitor.h"

#include <gflags/gflags.h>
#include <glog/logging.h>

// TODO: Use qt event loop with QSocketNotifier for async netlink reading
// TODO: Implement some kind of IPC
int main(int argc, char *argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;
    FLAGS_log_prefix = true;
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    VLOG(3) << "command line flags: " << gflags::CommandlineFlagsIntoString();
    RtNetlinkEthernetLinkAndIpAddressMonitor mon;

    return mon.run();
}
