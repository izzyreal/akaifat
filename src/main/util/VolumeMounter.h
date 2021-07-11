#pragma once

#include <fstream>

namespace akaifat::util {
class VolumeMounter {
public:
    static std::fstream mount(std::string volume);
    static void unmount(std::string volume);
};
}
