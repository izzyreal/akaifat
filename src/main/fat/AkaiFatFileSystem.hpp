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

public:
    AkaiFatFileSystem(std::shared_ptr<BlockDevice> device, bool readOnly,
            bool ignoreFatDifferences);
    
    AkaiFatFileSystem(std::shared_ptr<BlockDevice> device, bool readOnly)
    : AkaiFatFileSystem(std::move(device), readOnly, false) {}

    static AkaiFatFileSystem* read(std::shared_ptr<BlockDevice> device, bool readOnly);

    std::string getVolumeLabel();

    void setVolumeLabel(std::string label);

    void flush() override;
    
    std::shared_ptr<FsDirectory> getRoot() override;

    std::shared_ptr<BootSector> getBootSector();

    std::int64_t getFreeSpace() override;

    std::int64_t getTotalSpace() override;

    std::int64_t getUsableSpace() override;
};
}
