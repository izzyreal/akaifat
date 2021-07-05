#pragma once

#include "../AbstractFileSystem.hpp"

#include "AkaiFatLfnDirectory.hpp"
#include "Fat.hpp"
#include "Fat16BootSector.hpp"

#include <memory>

namespace akaifat::fat {
class AkaiFatFileSystem : public akaifat::AbstractFileSystem
{
private:
    std::shared_ptr<Fat> fat;
    std::shared_ptr<Fat16BootSector> bs;
    std::shared_ptr<AkaiFatLfnDirectory> rootDir;
    std::shared_ptr<AbstractDirectory> rootDirStore;
    long filesOffset;
            
public:
    AkaiFatFileSystem(BlockDevice* device, bool readOnly,
            bool ignoreFatDifferences);
    
    AkaiFatFileSystem(BlockDevice* api, bool readOnly)
    // Should ignoreFatDifferences be false?
    : AkaiFatFileSystem (api, readOnly, true) {}

    static AkaiFatFileSystem* read(BlockDevice* device, bool readOnly);

    long getFilesOffset();

    std::string getVolumeLabel();

    void setVolumeLabel(std::string label);

    std::shared_ptr<AbstractDirectory> getRootDirStore();
    
    void flush() override;
    
    std::shared_ptr<FsDirectory> getRoot() override;

    std::shared_ptr<BootSector> getBootSector();

    long getFreeSpace() override;

    long getTotalSpace() override;

    long getUsableSpace() override;
};
}
