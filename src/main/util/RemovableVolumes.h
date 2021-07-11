#ifdef __APPLE__
#include <DiskArbitration/DiskArbitration.h>
#endif

#include <vector>
#include <string>
#include <thread>

namespace akaifat {
namespace util {
class VolumeChangeListener {
public:
    VolumeChangeListener() = default;
    virtual void processChange(std::string change) { printf("Change: %s\n", change.c_str()); }
};

class RemovableVolumes {
public:
    RemovableVolumes() = default;
    ~RemovableVolumes();
    static std::vector<std::string> getBSDNames();
    
    void init();
    void addListener(VolumeChangeListener*);
    
private:
    bool running = false;
    std::thread changeListenerThread;
    std::vector<VolumeChangeListener*> listeners;
    
#ifdef __APPLE__
    static void diskAppeared(DADiskRef disk, void* context);
    static void diskDisappeared(DADiskRef disk, void* context);
#endif
    
};
}
}
