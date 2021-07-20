#if defined (__linux__)

#include "VolumeMounter.h"

#include <string>
#include <thread>

using namespace akaifat::util;

void demotePermissions(std::string driveLetter)
{
}

void repairPermissions(std::string driveLetter)
{
}

std::fstream VolumeMounter::mount(std::string driveLetter)
{
    std::fstream result;
    return result;
}

void VolumeMounter::unmount(std::string driveLetter)
{
    repairPermissions(driveLetter);
}

#endif
