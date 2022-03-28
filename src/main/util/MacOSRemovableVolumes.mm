#if defined (__APPLE__)
#include "RemovableVolumes.h"

#include <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>

using namespace akaifat::util;

void RemovableVolumes::diskAppeared(DADiskRef disk, void* context)
{
    CFDictionaryRef properties = DADiskCopyDescription(disk);

    auto ejectable = (NSString*)CFDictionaryGetValue(properties, kDADiskDescriptionMediaEjectableKey);
    auto volumeMountable = (NSString*) CFDictionaryGetValue(properties, kDADiskDescriptionVolumeMountableKey);
    auto mediaLeaf = (NSString*) CFDictionaryGetValue(properties, kDADiskDescriptionMediaLeafKey);
    
    if (mediaLeaf.intValue == 1 && ejectable.intValue == 1 && volumeMountable.intValue == 1)
    {
        NSString* volumeKind = (NSString*) CFDictionaryGetValue(properties, kDADiskDescriptionVolumeKindKey);
        auto volumeKindStr = volumeKind.UTF8String;
        bool isMsDos = strcmp(volumeKindStr, "msdos") == 0;

        if (!isMsDos) return;
        
        NSString* volumeType = (NSString*) CFDictionaryGetValue(properties, kDADiskDescriptionVolumeTypeKey);
        auto volumeTypeStr = volumeType.UTF8String;
        bool isFat16 = strcmp(volumeTypeStr, "MS-DOS (FAT16)") == 0;
        
        if (!isFat16) return;
        
        NSString* bsdName = (NSString*) CFDictionaryGetValue(properties, kDADiskDescriptionMediaBSDNameKey);
        CFNumberRef mediaSizeRef = (CFNumberRef) CFDictionaryGetValue(properties, kDADiskDescriptionMediaSizeKey);
        uint64_t mediaSize;
        CFNumberGetValue(mediaSizeRef, kCFNumberSInt64Type, &mediaSize);
        
        auto bsdNameStr = bsdName.UTF8String;
        printf("Disk appeared: %s, size: %lld\n", bsdNameStr, mediaSize);
        NSLog(@"Gone: %@", properties);

        auto that = (RemovableVolumes*)context;

        std::string volumeUUID;
        
        CFUUIDRef volumeUUIDKey = (CFUUIDRef) CFDictionaryGetValue(properties, kDADiskDescriptionVolumeUUIDKey);
        if(volumeUUIDKey)
        {
            CFStringRef uuid = CFUUIDCreateString(NULL, volumeUUIDKey);
            if(uuid==nullptr)
            {
                //DBG("WARNING: failed to convert UUID to CFString");
            } else {
                char tmp[256];

                if(!CFStringGetCString(uuid, tmp, sizeof(tmp), kCFStringEncodingUTF8)) {
                    //DBG("WARNING: failed to convert CFString to UTF8: " << __FILE__ << ":" << __LINE__);
                }
                volumeUUID = tmp;
                CFRelease(uuid);
            }
        }
        
        NSString* volumeName = (NSString*) CFDictionaryGetValue(properties, kDADiskDescriptionVolumeNameKey);
        std::string volumeNameStr = volumeName.UTF8String;
        
        for (auto& l : that->listeners)
            l->processChange(RemovableVolume{volumeUUID, bsdNameStr, volumeNameStr, mediaSize});
    }
}

void RemovableVolumes::diskDisappeared(DADiskRef disk, void* context)
{
}

void RemovableVolumes::init()
{
    running = true;
    
    changeListenerThread = std::thread([&]{
        DASessionRef session = DASessionCreate(kCFAllocatorDefault);
        DARegisterDiskAppearedCallback(session, NULL, diskAppeared, this);
        DARegisterDiskDisappearedCallback(session, NULL, diskDisappeared, this);
        DASessionScheduleWithRunLoop(session, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

        while (running)
        {
            CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        CFRelease(session);
    });
}
#endif
