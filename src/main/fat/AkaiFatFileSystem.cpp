#include "AkaiFatFileSystem.hpp"

#include "Fat16RootDirectory.hpp"

using namespace akaifat::fat;
using namespace akaifat;

AkaiFatFileSystem::AkaiFatFileSystem(
    BlockDevice* device,
    bool readOnly,
    bool ignoreFatDifferences
) : akaifat::AbstractFileSystem (readOnly),
    bs (std::dynamic_pointer_cast<Fat16BootSector>(BootSector::read(device)))
{                
    if (bs->getNrFats() <= 0) 
            throw std::runtime_error("boot sector says there are no FATs");
    
    filesOffset = bs->getFilesOffset();
    fat = Fat::read(bs, 0);

    if (!ignoreFatDifferences)
    {
        for (int i=1; i < bs->getNrFats(); i++)
        {
            auto tmpFat = Fat::read(bs, i);
        
            if (!fat->equals(tmpFat))
                throw std::runtime_error("FAT " + std::to_string(i) + " differs from FAT 0");
        }
    }

    rootDirStore = Fat16RootDirectory::read(bs, readOnly);

    rootDir = std::make_shared<AkaiFatLfnDirectory>(rootDirStore, fat, readOnly);
}

AkaiFatFileSystem* AkaiFatFileSystem::read(BlockDevice* device, bool readOnly)
{    
    return new AkaiFatFileSystem(device, readOnly);
}

long AkaiFatFileSystem::getFilesOffset()
{
    checkClosed();

    return filesOffset;
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

std::shared_ptr<AbstractDirectory> AkaiFatFileSystem::getRootDirStore()
{
    checkClosed();
    
    return rootDirStore;
}

void AkaiFatFileSystem::flush()
{
    checkClosed();
    
    if (bs->isDirty()) {
        bs->write();
    }
    
    for (int i = 0; i < bs->getNrFats(); i++) {
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

long AkaiFatFileSystem::getFreeSpace()
{
    checkClosed();

    return fat->getFreeClusterCount() * bs->getBytesPerCluster();
}

long AkaiFatFileSystem::getTotalSpace()
{
    checkClosed();
    return -1;
}

long AkaiFatFileSystem::getUsableSpace()
{
    checkClosed();

    return bs->getDataClusterCount() * bs->getBytesPerCluster();
}
