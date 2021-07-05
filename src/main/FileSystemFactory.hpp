#pragma once

#include "fat/AkaiFatFileSystem.hpp"

namespace akaifat {
class FileSystemFactory {
private:
    FileSystemFactory() { }
    
public:
    static FileSystem* createAkai(BlockDevice* device, bool readOnly) {
        return akaifat::fat::AkaiFatFileSystem::read(device, readOnly);
    }
    
};
}
