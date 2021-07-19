#pragma once

#ifdef __APPLE__
#include <DiskArbitration/DiskArbitration.h>
#elif _WIN32
#include <set>
#endif

#include <vector>
#include <string>
#include <thread>

namespace akaifat::util {
class VolumeChangeListener {
public:
    VolumeChangeListener() = default;
    virtual void processChange(std::string bsdName, int64_t mediaSize) = 0;
};

class RemovableVolumes {
public:
    RemovableVolumes() = default;
    ~RemovableVolumes()
    {
        running = false;

        while (!changeListenerThread.joinable())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        changeListenerThread.join();
    }

    void addListener(VolumeChangeListener* l)
    {
        listeners.emplace_back(l);
    }

    void init();
    
private:
    bool running = false;
    std::thread changeListenerThread;
    std::vector<VolumeChangeListener*> listeners;
    
#ifdef __APPLE__
    static void diskAppeared(DADiskRef disk, void* context);
    static void diskDisappeared(DADiskRef disk, void* context);
#elif _WIN32
    std::set<std::pair<std::string, unsigned long>> volumes;
    void detectChanges();
#endif

};
}
