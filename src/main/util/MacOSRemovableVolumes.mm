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
}

void RemovableVolumes::diskDisappeared(DADiskRef disk, void* context)
{
    printf("Bla bla bla\n");
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
