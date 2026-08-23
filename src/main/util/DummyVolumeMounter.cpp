#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if (defined(__APPLE__) && TARGET_OS_IOS) || defined(__ANDROID__)

#include "VolumeMounter.h"

using namespace akaifat::util;

std::fstream VolumeMounter::mount(std::string, bool)
{
    return {};
}

void VolumeMounter::unmount(std::string) {}

#endif
