#pragma once

#include "fat/AkaiFatFileSystem.hpp"

namespace akaifat {
class FileSystemFactory {
private:
    FileSystemFactory() { }
    
public:
    static FileSystem* createAkai(std::shared_ptr<BlockDevice> device, bool readOnly) {
        return akaifat::fat::AkaiFatFileSystem::read(device, readOnly);
    }
    
};
}
