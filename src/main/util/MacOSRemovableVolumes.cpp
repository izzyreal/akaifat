#if defined (__APPLE__)
#include "RemovableVolumes.h"

#include <Foundation/Foundation.h>

#include <DiskArbitration/DiskArbitration.h>

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

void diskAppeared(DADiskRef disk, void* context)
{
    CFDictionaryRef properties = DADiskCopyDescription(disk);
    
    auto ejectable = (NSString*)CFDictionaryGetValue(properties, kDADiskDescriptionMediaEjectableKey);
    auto volumeMountable = (NSString*) CFDictionaryGetValue(properties, kDADiskDescriptionVolumeMountableKey);
    
    if (ejectable.intValue == 1 && volumeMountable.intValue == 1)
    {
        NSString* bsdName = (NSString*) CFDictionaryGetValue(properties, kDADiskDescriptionMediaBSDNameKey);
        auto str = bsdName.UTF8String;
        printf("Disk appeared: %s\n", str);
    }
}

void diskDisappeared(DADiskRef disk, void* context)
{
    printf("Bla bla bla\n");
}

void RemovableVolumes::init()
{
    running = true;
    
    changeListenerThread = std::thread([&]{
        DASessionRef session = DASessionCreate(kCFAllocatorDefault);
        DARegisterDiskAppearedCallback(session, NULL, diskAppeared, NULL);
        DARegisterDiskDisappearedCallback(session, NULL, diskDisappeared, NULL);
        DASessionScheduleWithRunLoop(session, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

        while (running)
        {
            CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        CFRelease(session);
    });
}

std::vector<std::string> RemovableVolumes::getBSDNames()
{
    return {};
}
#endif
