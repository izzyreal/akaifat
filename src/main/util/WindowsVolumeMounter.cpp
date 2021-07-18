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


void demotePermissions(std::string driveLetter)
{
   int nRet = (int)ShellExecute(0, "runas", "powershell", "-command \"& {$NewAcl = [io.directory]::GetAccessControl('\\\\.\\F:'); $identity = 'BUILTIN\\Users'; $fileSystemRights = 'FullControl'; $type = 'Allow'; $fileSystemAccessRuleArgumentList = $identity, $fileSystemRights, $type; $fileSystemAccessRule = New-Object -TypeName System.Security.AccessControl.FileSystemAccessRule -ArgumentList $fileSystemAccessRuleArgumentList; $NewAcl.AddAccessRule($fileSystemAccessRule); [io.directory]::SetAccessControl('\\\\.\\F:',$NewAcl);}\"", 0, SW_SHOWNORMAL);

    if (nRet <= 32) {
        DWORD dw = GetLastError();
        char szMsg[250];
        FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM,
            0, dw, 0,
            szMsg, sizeof(szMsg),
            NULL
        );
        printf("Error while ShellExecute: %s\n", szMsg);
    }
}

void repairPermissions(std::string volumePath)
{
}

std::fstream VolumeMounter::mount(std::string driveLetter)
{
    demotePermissions(driveLetter);

    std::fstream result;
    char    fn[30];
    snprintf(fn, sizeof fn, "\\\\.\\%s:", driveLetter.c_str());
    HANDLE vol_handle = CreateFile(fn, GENERIC_ALL,
        FILE_SHARE_READ, NULL,
        OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS,
        NULL);

    if (vol_handle == INVALID_HANDLE_VALUE)
    {
        printf("Invalid handle for %s\n", fn);
        char* msg = strerror(errno);
        printf("strerror: %s\n", msg);
        return {};
    }

    int file_descriptor = _open_osfhandle((intptr_t)vol_handle, 0);

    if (file_descriptor != -1) {
        FILE* file = _fdopen(file_descriptor, "r+");

        if (file != NULL) {
            result = std::fstream(file);
        }
    }

    if (!result.is_open()) {
        char* msg = strerror(errno);
        printf("Failed to open fstream on %s\n", driveLetter.c_str());
        printf("Due to: %s\n", msg);
        return {};
    }

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
