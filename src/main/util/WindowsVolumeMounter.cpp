#if defined (_WIN32)

#include "VolumeMounter.h"

#include <Windows.h>
#include <io.h>

#include <string>

using namespace akaifat::util;

void unmountFromWindows(std::string volumePath)
{
}

void mountToWindows(std::string volumePath)
{
}


void demotePermissions(std::string volumePath)
{
}

void repairPermissions(std::string volumePath)
{
}

std::fstream VolumeMounter::mount(std::string driveLetter)
{    
    std::fstream result;
    /*
    char    fn[30];
    snprintf(fn, sizeof fn, "\\\\.\\%s:", driveLetter);

    HANDLE vol_handle = CreateFile(fn, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS,
        NULL);

    if (vol_handle == INVALID_HANDLE_VALUE)
    {
        // show error message and exit
    }

    int file_descriptor = _open_osfhandle((intptr_t)vol_handle, 0);

    if (file_descriptor != -1) {
        FILE* file = _fdopen(file_descriptor, "w");

        if (file != NULL) {
            result = std::fstream(file);

            if (!result.is_open()) {
                char* msg = strerror(errno);
                printf("Failed to open fstream on %s\n", driveLetter.c_str());
                printf("Due to: %s\n", msg);
                return {};
            }

            /* Cleanup
            result.close();

            file = NULL;
            file_descriptor = -1;
            vol_handle = INVALID_HANDLE_VALUE;
            */
        }
    }
    */
    return result;
}

void VolumeMounter::unmount(std::string bsdName)
{
    //if (!validateBSDName(bsdName))
      //  return;
    
    const auto volumePath = "/dev/" + bsdName;
    mountToWindows(volumePath);
    repairPermissions(volumePath);
}

#endif
