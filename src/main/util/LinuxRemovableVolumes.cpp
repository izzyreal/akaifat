#if defined (__linux__)
#include "RemovableVolumes.h"

using namespace akaifat::util;

std::vector<char> getDriveLetters()
{
    return {};
}

bool IsRemovable(char driveLetter)
{
    return false;
}

void RemovableVolumes::detectChanges()
{

}

void RemovableVolumes::init()
{
    running = true;

    changeListenerThread = std::thread([&]{
        while (running)
        {
            detectChanges();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });
}
#endif
