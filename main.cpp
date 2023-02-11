#include <iostream>

#include "NetlinkMonitorNlm.h"

#include <gflags/gflags.h>
#include <glog/logging.h>

int main(int argc, char *argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;
    FLAGS_log_prefix = true;
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    VLOG(3) << "command line flags: " << gflags::CommandlineFlagsIntoString();
    NetlinkMonitorNlm mon;

    return mon.run();
}
