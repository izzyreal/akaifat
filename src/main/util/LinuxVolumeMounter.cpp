#if defined (__linux__)

#include "VolumeMounter.h"

#include <string>
#include <thread>
#include <cstring>

using namespace akaifat::util;

void demotePermissions(std::string driveLetter)
{
}

void repairPermissions(std::string driveLetter)
{
    // set it back to 660
}

std::fstream VolumeMounter::mount(std::string bsdName, bool readOnly)
{
    std::fstream result;

    std::string cmd = "sudo chmod 626 " + bsdName;

    int err = system(cmd.c_str());

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

void VolumeMounter::unmount(std::string driveLetter)
{
    repairPermissions(driveLetter);
}

#endif
