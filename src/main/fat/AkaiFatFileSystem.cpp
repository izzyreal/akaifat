#include "AkaiFatFileSystem.hpp"

#include "Fat16RootDirectory.hpp"

using namespace akaifat::fat;
using namespace akaifat;

AkaiFatFileSystem::AkaiFatFileSystem(
                                     std::shared_ptr<BlockDevice> device,
                                     bool readOnly,
                                     bool ignoreFatDifferences
                                     ) : akaifat::AbstractFileSystem (readOnly),
                                    bs (std::dynamic_pointer_cast<Fat16BootSector>(BootSector::read(std::move(device))))
{
    if (bs->getNrFats() <= 0)
        throw std::runtime_error("boot sector says there are no FATs");

    fat = Fat::read(bs, 0);

    if (!ignoreFatDifferences)
    {
        for (std::int32_t i=1; i < bs->getNrFats(); i++)
        {
            auto tmpFat = Fat::read(bs, i);

            if (!fat->equals(tmpFat))
                throw std::runtime_error("FAT " + std::to_string(i) + " differs from FAT 0");
        }
    }

    rootDirStore = Fat16RootDirectory::read(bs, readOnly);

    rootDir = std::make_shared<AkaiFatLfnDirectory>(rootDirStore, fat, readOnly);
    rootDir->parseLfn();
}

AkaiFatFileSystem* AkaiFatFileSystem::read(std::shared_ptr<BlockDevice> device, bool readOnly)
{
    return new AkaiFatFileSystem(std::move(device), readOnly);
}

std::string AkaiFatFileSystem::getVolumeLabel()
{
    checkClosed();
    return rootDirStore->getLabel();
}

void AkaiFatFileSystem::setVolumeLabel(std::string label)
{

    checkClosed();
    checkReadOnly();

    rootDirStore->setLabel(label);
    bs->setVolumeLabel(label);
}

void AkaiFatFileSystem::flush()
{
    checkClosed();

    if (bs->isDirty()) {
        bs->write();
    }

    for (std::int32_t i = 0; i < bs->getNrFats(); i++) {
        fat->writeCopy(bs->getFatOffset(i));
    }

    rootDir->flush();
}

std::shared_ptr<FsDirectory> AkaiFatFileSystem::getRoot()
{
    checkClosed();

    return rootDir;
}

std::shared_ptr<BootSector> AkaiFatFileSystem::getBootSector()
{
    checkClosed();

    return bs;
}

std::int64_t AkaiFatFileSystem::getFreeSpace()
{
    checkClosed();

    return fat->getFreeClusterCount() * bs->getBytesPerCluster();
}

std::int64_t AkaiFatFileSystem::getTotalSpace()
{
    checkClosed();
    return -1;
}

std::int64_t AkaiFatFileSystem::getUsableSpace()
{
    checkClosed();

    return bs->getDataClusterCount() * bs->getBytesPerCluster();
}
