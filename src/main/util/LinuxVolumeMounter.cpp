#if defined (_WIN32)

#include "VolumeMounter.h"

#include <Windows.h>
#include <io.h>

#include <string>
#include <thread>

using namespace akaifat::util;

void demotePermissions(std::string driveLetter)
{
    std::string cmd = "-command \"& {\
        $NewAcl = [io.directory]::GetAccessControl('\\\\.\\" + driveLetter + ":');\
        $identity = 'BUILTIN\\Users';\
        $fileSystemRights = 'FullControl';\
        $type = 'Allow';\
        $fileSystemAccessRuleArgumentList = $identity, $fileSystemRights, $type;\
        $fileSystemAccessRule = New-Object -TypeName System.Security.AccessControl.FileSystemAccessRule -ArgumentList $fileSystemAccessRuleArgumentList;\
        $NewAcl.AddAccessRule($fileSystemAccessRule);\
        [io.directory]::SetAccessControl('\\\\.\\" + driveLetter + ":',$NewAcl);}\"";

   int nRet = (int)ShellExecute(0,
       "runas",
       "powershell",
       cmd.c_str(),
       0,
       SW_HIDE);

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
    else
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void repairPermissions(std::string driveLetter)
{
    std::string cmd = "-command \"& {\
        $NewAcl = [io.directory]::GetAccessControl('\\\\.\\" + driveLetter + ":');\
        $identity = 'BUILTIN\\Users';\
        $fileSystemRights = 'FullControl';\
        $type = 'Allow';\
        $fileSystemAccessRuleArgumentList = $identity, $fileSystemRights, $type;\
        $fileSystemAccessRule = New-Object -TypeName System.Security.AccessControl.FileSystemAccessRule -ArgumentList $fileSystemAccessRuleArgumentList;\
        $NewAcl.AddAccessRule($fileSystemAccessRule);\
        [io.directory]::RemoveAccessControl('\\\\.\\" + driveLetter + ":',$NewAcl);}\"";

    int nRet = (int)ShellExecute(0,
        "runas",
        "powershell",
        cmd.c_str(),
        0,
        SW_HIDE);

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
    else
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
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

void VolumeMounter::unmount(std::string driveLetter)
{
    repairPermissions(driveLetter);
}

#endif
