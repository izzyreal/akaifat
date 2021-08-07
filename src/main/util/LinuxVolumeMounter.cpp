#if defined (__linux__)

#include "VolumeMounter.h"

#include <string>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>

using namespace akaifat::util;

std::string getCurrentUser()
{
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw) return std::string(pw->pw_name);
    return "";
}

int demotePermissions(std::string bsdName)
{
    std::string currentUser = getCurrentUser();

    struct stat info;
    stat(bsdName.c_str(), &info);
    struct passwd *pw = getpwuid(info.st_uid);
    char* currentOwnerUserName = pw->pw_name;

    if (currentOwnerUserName != currentUser)
    {
        std::string cmd = "pkexec chown " + currentUser + " " + bsdName;
        system(cmd.c_str());
    }

    std::string cmd = "chmod 626 " + bsdName;
    return system(cmd.c_str());
}

int repairPermissions(std::string bsdName)
{
    std::string cmd = "chmod 660 " + bsdName;
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
