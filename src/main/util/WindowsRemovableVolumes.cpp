#if defined (_WIN32)
#include "RemovableVolumes.h"

#include <Windows.h>
#include <cfgmgr32.h>

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

void RemovableVolumes::diskAppeared(void* disk, void* context)
{
    /*
    CFDictionaryRef properties = DADiskCopyDescription(disk);

    auto ejectable = (NSString*)CFDictionaryGetValue(properties, kDADiskDescriptionMediaEjectableKey);
    auto volumeMountable = (NSString*) CFDictionaryGetValue(properties, kDADiskDescriptionVolumeMountableKey);

    if (ejectable.intValue == 1 && volumeMountable.intValue == 1)
    {
        NSString* bsdName = (NSString*) CFDictionaryGetValue(properties, kDADiskDescriptionMediaBSDNameKey);
        CFNumberRef s1 = (CFNumberRef) CFDictionaryGetValue(properties, kDADiskDescriptionMediaSizeKey);
        int64_t mediaSize;
        CFNumberGetValue(s1, kCFNumberSInt64Type, &mediaSize);
        auto bsdNameStr = bsdName.UTF8String;
        printf("Disk appeared: %s, size: %lld\n", bsdNameStr, mediaSize);

        auto that = (RemovableVolumes*)context;

        for (auto& l : that->listeners)
            l->processChange(bsdNameStr, mediaSize);
    }
    */
}

void RemovableVolumes::diskDisappeared(void* disk, void* context)
{
    printf("Bla bla bla\n");
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

bool IsRemovable2(char driveLetter)
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
            printf("%s IS removable\n", volumePath.c_str());
            result = true;
        }
        else
        {
            printf("%s is NOT removable\n", volumePath.c_str());
        }
    }
    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;
    return result;
}

bool IsRandomAccessible(char driveLetter)
{
    std::string volumePath = "\\\\.\\" + std::string(1, driveLetter) + ":";

    HANDLE hFile = CreateFile(volumePath.c_str(), GENERIC_ALL, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
    
    if (hFile != INVALID_HANDLE_VALUE)
    {
        printf("%s IS random-accessible\n", volumePath.c_str());
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
        return true;
    }
    else
    {
        printf("%s is NOT random-accessible\n", volumePath.c_str());
        return false;
    }
}

ULONG IsRemovable(HANDLE hDisk, BOOLEAN& RemovableMedia, DWORD& Size)
{
    STORAGE_PROPERTY_QUERY spq = { StorageDeviceProperty, PropertyStandardQuery };

    STORAGE_DEVICE_DESCRIPTOR sdd;

    ULONG rcb;
    if (DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(spq), &sdd, sizeof(sdd), &rcb, 0))
    {
        RemovableMedia = sdd.RemovableMedia;
        Size = sdd.Size;
        return NOERROR;
    }

    return GetLastError();
}

void EnumDisks()
{
    ULONG len;

    if (!CM_Get_Device_Interface_List_SizeW(&len, const_cast<GUID*>(&GUID_DEVINTERFACE_DISK), 0, CM_GET_DEVICE_INTERFACE_LIST_PRESENT))
    {
        PWSTR buf = (PWSTR)alloca(len << 1);
        if (!CM_Get_Device_Interface_ListW(const_cast<GUID*>(&GUID_DEVINTERFACE_DISK), 0, buf, len, CM_GET_DEVICE_INTERFACE_LIST_PRESENT))
        {
            while (*buf)
            {
                HANDLE hDisk = CreateFileW(buf, 0, FILE_SHARE_VALID_FLAGS, 0, OPEN_EXISTING, 0, 0);

                if (hDisk != INVALID_HANDLE_VALUE)
                {
                    BOOLEAN RemovableMedia;
                    DWORD Size;
                    if (!IsRemovable(hDisk, RemovableMedia, Size))
                    {
                        printf("%u %lu %S\n", Size, RemovableMedia, buf);
                    }

                    CloseHandle(hDisk);
                }
                buf += wcslen(buf) + 1;
            }
        }
    }
}

void EnumDisks2()
{
    auto driveLetters = getDriveLetters();
    
    for (auto driveLetter : driveLetters)
    {
        printf("Checking %c\n", driveLetter);
        auto removable = IsRemovable2(driveLetter);

        if (!removable) continue;

        std::string path = std::string(1, driveLetter) + ":\\";
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
            printf("Could get %c\n", driveLetter);
            unsigned long sectorsPerCluster = (unsigned long) lpSectorsPerCluster;
            unsigned long bytesPerSector = (unsigned long)lpBytesPerSector;
            unsigned long numberOfFreeClusters = (unsigned long)lpNumberOfFreeClusters;
            unsigned long totalNumberOfClusters = (unsigned long)lpTotalNumberOfClusters;
            
            printf("secPerClust: %lu, bytPerSec: %lu, freeClust: %lu, totalClust: %lu\n",
                sectorsPerCluster, bytesPerSector, numberOfFreeClusters, totalNumberOfClusters);

            IsRandomAccessible(driveLetter);
        }
    }
}

void RemovableVolumes::init()
{
    running = true;
    
    changeListenerThread = std::thread([&]{
        while (running)
        {
            EnumDisks2();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
}
#endif
