#ifdef __APPLE__
#include <TargetConditionals.h>
#ifdef TARGET_IPHONE_SIMULATOR

#include "VolumeMounter.h"

using namespace akaifat::util;

std::fstream VolumeMounter::mount(std::string volume, bool readOnly) { return std::fstream(); }
void VolumeMounter::unmount(std::string volume){}

#endif
#endif
