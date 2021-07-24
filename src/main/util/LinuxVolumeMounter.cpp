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
}

std::fstream VolumeMounter::mount(std::string bsdName, bool readOnly)
{
    std::fstream result;

    printf("\n\n=========VolumeMounter mount bsdName: %s\n", bsdName.c_str());

    std::string cmd = "sudo chmod 626 " + bsdName;

    int err = system(cmd.c_str());
    printf("\n\n===============Err: %i\n", err);

    if (err == 0)
    {
        result.open(bsdName.c_str(), std::ios_base::in | std::ios_base::out);

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
