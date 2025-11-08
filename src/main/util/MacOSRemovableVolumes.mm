#if defined (__APPLE__)
#include "RemovableVolumes.h"

#include <Foundation/Foundation.h>

using namespace akaifat::util;

void RemovableVolumes::diskAppeared(DADiskRef disk, void* context)
{
    CFDictionaryRef properties = DADiskCopyDescription(disk);

    auto ejectable = (NSString*)CFDictionaryGetValue(properties, kDADiskDescriptionMediaEjectableKey);
    auto volumeMountable = (NSString*)CFDictionaryGetValue(properties, kDADiskDescriptionVolumeMountableKey);
    auto mediaLeaf = (NSString*)CFDictionaryGetValue(properties, kDADiskDescriptionMediaLeafKey);

    if (mediaLeaf.intValue == 1 && ejectable.intValue == 1 && volumeMountable.intValue == 1)
    {
        auto volumeKind = (NSString*)CFDictionaryGetValue(properties, kDADiskDescriptionVolumeKindKey);
        auto volumeKindStr = volumeKind.UTF8String;
        bool isMsDos = strcmp(volumeKindStr, "msdos") == 0;

        if (!isMsDos)
        {
            CFRelease(properties);
            return;
        }

        auto volumeType = (NSString*)CFDictionaryGetValue(properties, kDADiskDescriptionVolumeTypeKey);
        auto volumeTypeStr = volumeType.UTF8String;
        bool isFat16 = strcmp(volumeTypeStr, "MS-DOS (FAT16)") == 0;

        if (!isFat16)
        {
            CFRelease(properties);
            return;
        }

        auto bsdName = (NSString*)CFDictionaryGetValue(properties, kDADiskDescriptionMediaBSDNameKey);
        auto mediaSizeRef = (CFNumberRef)CFDictionaryGetValue(properties, kDADiskDescriptionMediaSizeKey);
        uint64_t mediaSize;
        CFNumberGetValue(mediaSizeRef, kCFNumberSInt64Type, &mediaSize);

        auto bsdNameStr = bsdName.UTF8String;
        printf("Disk appeared: %s, size: %lld\n", bsdNameStr, mediaSize);
        NSLog(@"Gone: %@", properties);

        auto that = static_cast<RemovableVolumes*>(context);

        std::string volumeUUID;
        auto volumeUUIDKey = (CFUUIDRef)CFDictionaryGetValue(properties, kDADiskDescriptionVolumeUUIDKey);
        if (volumeUUIDKey)
        {
            CFStringRef uuid = CFUUIDCreateString(NULL, volumeUUIDKey);
            if (uuid != nullptr)
            {
                char tmp[256];
                if (CFStringGetCString(uuid, tmp, sizeof(tmp), kCFStringEncodingUTF8))
                    volumeUUID = tmp;
                CFRelease(uuid);
            }
        }

        auto volumeName = (NSString*)CFDictionaryGetValue(properties, kDADiskDescriptionVolumeNameKey);
        std::string volumeNameStr = volumeName.UTF8String;

        // Copy listeners under lock, then call outside lock
        std::vector<VolumeChangeListener*> snapshot;
        {
            std::lock_guard<std::mutex> lk(that->listenersMutex);
            snapshot = that->listeners;
        }

        for (auto* l : snapshot)
        {
            l->processChange(RemovableVolume{volumeUUID, bsdNameStr, volumeNameStr, mediaSize});
        }
    }

    CFRelease(properties);
}

void RemovableVolumes::diskDisappeared(DADiskRef disk, void* context)
{
}

void RemovableVolumes::init()
{
    std::lock_guard<std::mutex> lk(listenersMutex);
    if (running.load()) return;
    running.store(true);

    changeListenerThread = std::thread([this] {
        DASessionRef session = DASessionCreate(kCFAllocatorDefault);
        DARegisterDiskAppearedCallback(session, NULL, diskAppeared, this);
        DARegisterDiskDisappearedCallback(session, NULL, diskDisappeared, this);
        DASessionScheduleWithRunLoop(session, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

        while (running.load())
        {
            CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        CFRelease(session);
    });
}
#endif
