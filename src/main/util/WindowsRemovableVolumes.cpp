#if defined (_WIN32)
#include "RemovableVolumes.h"

#include <Windows.h>
#include <cfgmgr32.h>

#pragma comment(lib, "cfgmgr32.lib")

#define FILE_SHARE_VALID_FLAGS 0x00000007

using namespace akaifat::util;

RemovableVolumes::~RemovableVolumes()
{
    running = false;
    
    while (!changeListenerThread.joinable())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    changeListenerThread.join();
}

void RemovableVolumes::addListener(VolumeChangeListener* l)
{
    listeners.emplace_back(l);
}

std::vector<char> getDriveLetters()
{
    std::vector<char> result;
    CHAR cDriveLetter = 'A';
    DWORD dwDrivemap = GetLogicalDrives();

    while (cDriveLetter <= 'Z')
    {
        if (0 < (dwDrivemap & 0x00000001L))
        {
            result.push_back(cDriveLetter);
        }
        cDriveLetter++;
        dwDrivemap = dwDrivemap >> 1;
    }
    return result;
}

bool IsRemovable(char driveLetter)
{
    bool result = false;
    std::string volumePath = "\\\\.\\" + std::string(1, driveLetter) + ":";

    HANDLE hFile = CreateFile(volumePath.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    STORAGE_PROPERTY_QUERY StoragePropertyQuery;
    StoragePropertyQuery.PropertyId = StorageDeviceProperty;
    StoragePropertyQuery.QueryType = PropertyStandardQuery;
    BYTE Buffer[1024];
    LPDWORD BytesReturned = 0;

    if (DeviceIoControl(hFile, IOCTL_STORAGE_QUERY_PROPERTY, &StoragePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY), Buffer, 1024, BytesReturned, NULL))
    {
        PSTORAGE_DEVICE_DESCRIPTOR StorageDeviceDescriptor = (PSTORAGE_DEVICE_DESCRIPTOR)Buffer;
        if (StorageDeviceDescriptor->RemovableMedia)
        {
            result = true;
        }
    }

    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;
    return result;
}

void RemovableVolumes::detectChanges()
{
    auto driveLetters = getDriveLetters();
    
    for (auto driveLetter : driveLetters)
    {
        auto removable = IsRemovable(driveLetter);

        if (!removable) continue;

        std::string driveLetterStr = std::string(1, driveLetter);

        std::string path = driveLetterStr + ":\\";
        LPCSTR  lpRootPathName = path.c_str();
        DWORD lpSectorsPerCluster = 0;
        DWORD lpBytesPerSector = 0;
        DWORD lpNumberOfFreeClusters = 0;
        DWORD lpTotalNumberOfClusters = 0;
        
        if (GetDiskFreeSpaceA(
            lpRootPathName,
            &lpSectorsPerCluster,
            &lpBytesPerSector,
            &lpNumberOfFreeClusters,
            &lpTotalNumberOfClusters
        )) {
            unsigned long sectorsPerCluster = (unsigned long) lpSectorsPerCluster;
            unsigned long bytesPerSector = (unsigned long)lpBytesPerSector;
            unsigned long numberOfFreeClusters = (unsigned long)lpNumberOfFreeClusters;
            unsigned long totalNumberOfClusters = (unsigned long)lpTotalNumberOfClusters;
            
            unsigned long mediaSize = bytesPerSector * sectorsPerCluster * totalNumberOfClusters;

            if (volumes.emplace(std::pair<std::string, unsigned long>{ driveLetterStr, mediaSize }).second)
            {
                for (auto& l : listeners)
                    l->processChange(driveLetterStr, mediaSize);
            }
        }
    }
}

void RemovableVolumes::init()
{
    running = true;
    
    changeListenerThread = std::thread([&]{
        while (running)
        {
            detectChanges();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });
}
#endif
