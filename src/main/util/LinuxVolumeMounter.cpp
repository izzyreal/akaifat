#if defined (__linux__)

#include "VolumeMounter.h"

#include <string>
#include <thread>
#include <cstring>

using namespace akaifat::util;

int demotePermissions(std::string bsdName)
{
    std::string cmd = "pkexec chmod 626 " + bsdName;
    return system(cmd.c_str());
}

int repairPermissions(std::string bsdName)
{
    std::string cmd = "pkexec chmod 660 " + bsdName;
    return system(cmd.c_str());
}

std::fstream VolumeMounter::mount(std::string bsdName, bool readOnly)
{
    std::fstream result;

    int err = demotePermissions(bsdName);

    if (err == 0)
    {
        auto flags = readOnly ? (std::ios_base::in) : (std::ios_base::in | std::ios_base::out);

        result.open(bsdName.c_str(), flags);

        if (!result.is_open()) {
            char* msg = strerror(errno);
            printf("Failed to open fstream on %s\n", bsdName.c_str());
            printf("Due to: %s\n", msg);
            return {};
        }
    }

    return result;
}

void VolumeMounter::unmount(std::string bsdName)
{
    repairPermissions(bsdName);
}

#endif
