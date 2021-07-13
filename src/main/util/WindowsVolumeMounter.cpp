#if defined (_WIN32)

#include "VolumeMounter.h"

#include <string>

using namespace akaifat::util;

void unmountFromWindows(std::string volumePath)
{
    //const std::string cmd = "diskutil unmount " + volumePath;
    //system(cmd.c_str());
}

void mountToWindows(std::string volumePath)
{
    //const std::string cmd = "diskutil mount " + volumePath;
    //system(cmd.c_str());
}


void demotePermissions(std::string volumePath)
{
    /*
    std::string cmdStr = "/bin/chmod";
    char* cmd = const_cast<char*>(cmdStr.c_str());
    
    std::string titleStr = "Please approve temporary permission demotion";
    char* title = const_cast<char*>(titleStr.c_str());

    char* argv[0];
    std::string arg1Str = "626";
    std::string arg2Str = volumePath;
    
    char* icon = NULL;

    char* commandArgs[3];
    
    commandArgs[0] = const_cast<char*>(arg1Str.c_str());
    commandArgs[1] = const_cast<char*>(arg2Str.c_str());
    commandArgs[2] = nil;
    
    cocoasudo(cmd, commandArgs, icon, title);
    */
}

void repairPermissions(std::string volumePath)
{
    /*
    std::string cmdStr = "/bin/chmod";
    char* cmd = const_cast<char*>(cmdStr.c_str());
    
    std::string titleStr = "Please approve permission repair of " + volumePath;
    char* title = const_cast<char*>(titleStr.c_str());

    char* argv[0];
    std::string arg1Str = "640";
    std::string arg2Str = volumePath;
    
    char* icon = NULL;

    char* commandArgs[3];
    
    commandArgs[0] = const_cast<char*>(arg1Str.c_str());
    commandArgs[1] = const_cast<char*>(arg2Str.c_str());
    commandArgs[2] = nil;
    
    cocoasudo(cmd, commandArgs, icon, title);
    */
}

bool validateBSDName(std::string bsdName)
{
    const static unsigned int MIN_BSD_NAME_LENGTH = 5;
    bool valid = true;
    
    if (bsdName.length() < MIN_BSD_NAME_LENGTH) {
        printf("bsdName length < 5\n");
        valid = false;
    }
    
    const bool validStart = bsdName.substr(0, 4) == "disk" || bsdName.substr(0, 5) == "rdisk";
    
    if (!validStart)
    {
        printf("%s is an illegal BSD name. bsdName must begin with disk or rdisk\n", bsdName.c_str());
        valid = false;
    }
    
    /*
    const auto volumePath = "/dev/" + bsdName;
    
    if (!file_exists(volumePath.c_str()))
    {
        printf("Volume path %s does not exist\n", volumePath.c_str());
        valid = false;
    }
    */
    
    return valid;
}

std::fstream VolumeMounter::mount(std::string bsdName)
{
    if (!validateBSDName(bsdName))
        return {};
    
    const auto volumePath = "/dev/" + bsdName;

    unmountFromWindows(volumePath);
    demotePermissions(volumePath);
    
    printf("Volume path %s exists\n", volumePath.c_str());
    printf("Attempting Akai FAT16 mount...\n");
    
    std::fstream result;
    
    result.open(volumePath.c_str(), std::ios_base::in | std::ios_base::out);
    
    if (!result.is_open()) {
        char* msg = strerror(errno);
        printf("Failed to open fstream on %s\n", volumePath.c_str());
        printf("Due to: %s\n", msg);
        return {};
    }
    
    return result;
}

void VolumeMounter::unmount(std::string bsdName)
{
    if (!validateBSDName(bsdName))
        return;
    
    const auto volumePath = "/dev/" + bsdName;
    mountToWindows(volumePath);
    repairPermissions(volumePath);
}

#endif
