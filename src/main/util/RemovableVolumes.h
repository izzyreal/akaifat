#pragma once

#ifdef __APPLE__
    #include <TargetConditionals.h>
    #if !defined(TARGET_IPHONE_SIMULATOR)
        #include <DiskArbitration/DiskArbitration.h>
    #endif
#elif _WIN32
#include <set>
#elif __linux__
#include <pthread.h>
#include <udisks/udisks.h>
#endif

#include <vector>
#include <string>
#include <thread>

namespace akaifat::util {
struct RemovableVolume {
    std::string volumeUUID;
    std::string deviceName;
    std::string volumeName;
    uint64_t mediaSize;
};

class VolumeChangeListener {
public:
    VolumeChangeListener() = default;
    virtual void processChange(RemovableVolume) = 0;
};

class RemovableVolumes {
public:
    RemovableVolumes() = default;
    ~RemovableVolumes()
    {
        running = false;

//        while (!changeListenerThread.joinable())
//        {
//            std::this_thread::sleep_for(std::chrono::milliseconds(10));
//        }
//
//        changeListenerThread.join();
    }

    void addListener(VolumeChangeListener* l)
    {
        listeners.emplace_back(l);
    }
#if defined(__APPLE__) && defined(TARGET_IPHONE_SIMULATOR)
    void init(){}
#else
    void init();
#endif

private:
    bool running = false;
    std::thread changeListenerThread;
    std::vector<VolumeChangeListener*> listeners;

#ifdef __APPLE__
#if !defined(TARGET_IPHONE_SIMULATOR)
    static void diskAppeared(DADiskRef disk, void* context);
    static void diskDisappeared(DADiskRef disk, void* context);
#endif
#elif _WIN32
    std::set<std::string> volumes;
    void detectChanges();
#elif __linux__
    static void on_object_added(GDBusObjectManager *manager,
                            GDBusObject *dbus_object, gpointer user_data);

#endif

};
}
