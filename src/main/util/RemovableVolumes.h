#pragma once

#ifdef __APPLE__
    #include <TargetConditionals.h>
    #if !TARGET_OS_IOS
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
#include <atomic>
#include <mutex>

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
        running.store(false);

#if !TARGET_OS_IOS
        if (changeListenerThread.joinable())
        {
            changeListenerThread.join();
        }
#endif
    }

    void addListener(VolumeChangeListener* l)
    {
        listeners.emplace_back(l);
    }
#if (defined __APPLE__ && TARGET_OS_IOS)
    void init(){}
#else
    void init();
#endif

private:
    std::atomic<bool> running{false};
    std::thread changeListenerThread;
    std::vector<VolumeChangeListener*> listeners;
    std::mutex listenersMutex;

#ifdef __APPLE__
#if !TARGET_OS_IOS
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
