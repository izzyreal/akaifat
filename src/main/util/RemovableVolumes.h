#include <vector>
#include <string>
#include <thread>

namespace akaifat {
namespace util {
class RemovableVolumes {
public:
    RemovableVolumes() = default;
    ~RemovableVolumes();
    static std::vector<std::string> getBSDNames();
    
    void init();
    
private:
    bool running = false;
    std::thread changeListenerThread;
    
};
}
}
