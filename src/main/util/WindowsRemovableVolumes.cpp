#if defined (_WIN32)
#include "RemovableVolumes.h"

#include <Windows.h>
#include <cfgmgr32.h>

#pragma comment(lib, "cfgmgr32.lib")

#define FILE_SHARE_VALID_FLAGS 0x00000007

using namespace akaifat::util;

std::vector<char> getDriveLetters()
{
    std::vector<char> result;
    CHAR cDriveLetter = 'A';
    DWORD dwDrivemap = GetLogicalDrives();

    while (cDriveLetter <= 'Z')
    {
        if (0 < (dwDrivemap & 0x00000001L))
            result.push_back(cDriveLetter);

        cDriveLetter++;
        dwDrivemap >>= 1;
    }
    return result;
}

bool IsRemovable(char driveLetter)
{
    bool result = false;
    std::string volumePath = "\\\\.\\" + std::string(1, driveLetter) + ":";

    HANDLE hFile = CreateFile(volumePath.c_str(), FILE_READ_ATTRIBUTES,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    STORAGE_PROPERTY_QUERY query = {};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;
    BYTE buffer[1024];
    DWORD bytesReturned = 0;

    if (DeviceIoControl(hFile, IOCTL_STORAGE_QUERY_PROPERTY,
                        &query, sizeof(query),
                        buffer, sizeof(buffer),
                        &bytesReturned, NULL))
    {
        auto* desc = reinterpret_cast<PSTORAGE_DEVICE_DESCRIPTOR>(buffer);
        if (desc->RemovableMedia)
            result = true;
    }

    CloseHandle(hFile);
    return result;
}

void RemovableVolumes::detectChanges()
{
    auto driveLetters = getDriveLetters();

    for (auto driveLetter : driveLetters)
    {
        if (!IsRemovable(driveLetter)) continue;

        std::string driveLetterStr(1, driveLetter);
        std::string path = driveLetterStr + ":\\";
        LPCSTR rootPath = path.c_str();

        DWORD sectorsPerCluster = 0, bytesPerSector = 0;
        DWORD freeClusters = 0, totalClusters = 0;

        if (!GetDiskFreeSpace(rootPath, &sectorsPerCluster, &bytesPerSector,
                              &freeClusters, &totalClusters))
            continue;

        uint64_t mediaSize = static_cast<uint64_t>(bytesPerSector)
                           * sectorsPerCluster * totalClusters;
        if (mediaSize == 0) continue;

        char volumeName[11] = {0};
        char fileSystemName[8] = {0};
        std::string volumeNameStr;

        if (GetVolumeInformation(rootPath, volumeName, sizeof(volumeName),
                                 nullptr, nullptr, nullptr, fileSystemName,
                                 sizeof(fileSystemName)))
        {
            if (strcmp(fileSystemName, "FAT") != 0) continue;
            volumeNameStr = volumeName;
        }

        char volumeGUID[50] = {0};
        std::string volumeGUIDStr;
        if (GetVolumeNameForVolumeMountPoint(rootPath, volumeGUID, sizeof(volumeGUID)))
            volumeGUIDStr = volumeGUID;

        bool isNew = false;
        {
            std::lock_guard<std::mutex> lock(listenersMutex);
            isNew = volumes.emplace(volumeGUIDStr).second;
        }

        if (isNew)
        {
            std::vector<VolumeChangeListener*> snapshot;
            {
                std::lock_guard<std::mutex> lock(listenersMutex);
                snapshot = listeners;
            }

            for (auto* l : snapshot)
                l->processChange(RemovableVolume{volumeGUIDStr, driveLetterStr, volumeNameStr, mediaSize});
        }
    }
}

void RemovableVolumes::init()
{
    std::lock_guard<std::mutex> lk(listenersMutex);
    if (running.load()) return;
    running.store(true);

    changeListenerThread = std::thread([this] {
        while (running.load())
        {
            detectChanges();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });
}
#endif
