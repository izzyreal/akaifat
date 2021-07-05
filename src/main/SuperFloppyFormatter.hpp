#include "BlockDevice.hpp"
#include "fat/FatType.hpp"

#include "fat/BootSector.hpp"
#include "fat/Fat16RootDirectory.hpp"

#include <memory>
#include <string>

namespace akaifat {
class SuperFloppyFormatter {
    
private:
    static const int MEDIUM_DESCRIPTOR_HD = 0xf8;
    static const int DEFAULT_FAT_COUNT = 2;
    static const int DEFAULT_SECTORS_PER_TRACK = 32;
    static const int DEFAULT_HEADS = 64;
    static std::string& DEFAULT_OEM_NAME() { static std::string result = "        "; return result; }
    static const int MAX_DIRECTORY = 512;
    
    BlockDevice& device;
    int fatCount;
    std::string label;
    std::string oemName;
    std::shared_ptr<akaifat::fat::FatType> fatType;
    int sectorsPerCluster;
    int reservedSectors;
    
    int sectorsPerCluster16() {
        if (reservedSectors != 1) throw std::runtime_error(
                                                           "number of reserved sectors must be 1");
        
        if (fatCount != 2) throw std::runtime_error(
                                                    "number of FATs must be 2");
        
        long sectors = device.getSize() / device.getSectorSize();
        
        if (sectors <= 8400) throw std::runtime_error(
                                                      "disk too small for FAT16 (" + std::to_string(sectors) + ")");
        
        if (sectors > 4194304) throw std::runtime_error(
                                                        "disk too large for FAT16");
        
        int result;
        
        if (sectors > 2097152) result = 64;
        else if (sectors > 1048576) result = 32;
        else if (sectors > 524288) result = 16;
        else if (sectors > 262144) result = 8;
        else if (sectors > 32680) result = 4;
        else result = 2;
        
        return result;
    }
    
    void initBootSector(akaifat::fat::BootSector& bs) {
        bs.init();
        auto label = fatType->getLabel();
        bs.setFileSystemTypeLabel(label);
        bs.setNrReservedSectors(reservedSectors);
        bs.setNrFats(fatCount);
        bs.setSectorsPerCluster(sectorsPerCluster);
        bs.setMediumDescriptor(MEDIUM_DESCRIPTOR_HD);
        bs.setSectorsPerTrack(DEFAULT_SECTORS_PER_TRACK);
        bs.setNrHeads(DEFAULT_HEADS);
        bs.setOemName(oemName);
    }
    
    int rootDirectorySize(int bps, int nbTotalSectors) {
        const int totalSize = bps * nbTotalSectors;
        if (totalSize >= MAX_DIRECTORY * 5 * 32) {
            return MAX_DIRECTORY;
        } else {
            return totalSize / (5 * 32);
        }
    }
    
    int sectorsPerFat(int rootDirEntries, int totalSectors) {
            const int bps = device.getSectorSize();
            const int rootDirSectors =
                    ((rootDirEntries * 32) + (bps - 1)) / bps;
            const long tmp1 =
                    totalSectors - (reservedSectors + rootDirSectors);
            int tmp2 = (256 * sectorsPerCluster) + fatCount;

            auto result = (int) ((tmp1 + (tmp2 - 1)) / tmp2);
            
            return result;
        }
    
public:
    void setVolumeLabel(std::string _label) {
        label = _label;
    }
    
    akaifat::fat::AkaiFatFileSystem* format() {
        const int sectorSize = device.getSectorSize();
        const int totalSectors = (int)(device.getSize() / sectorSize);
        
        if (sectorsPerCluster == 0) throw std::runtime_error("sectorsPerCluster == 0");
        
        auto f16bs = std::make_shared<akaifat::fat::Fat16BootSector>(&device);
        initBootSector(*f16bs.get());
        
        int rootDirEntries = rootDirectorySize(device.getSectorSize(), totalSectors);
        
        f16bs->setRootDirEntryCount(rootDirEntries);
        f16bs->setSectorsPerFat(sectorsPerFat(rootDirEntries, totalSectors));
        
        if (!label.empty()) f16bs->setVolumeLabel(label);
        
        auto fat = akaifat::fat::Fat::create(f16bs, 0);
        
        auto rootDirStore = akaifat::fat::Fat16RootDirectory::create(f16bs);
        
        akaifat::fat::AkaiFatLfnDirectory rootDir(rootDirStore, fat, false);
        
        rootDir.flush();
        
        for (int i = 0; i < f16bs->getNrFats(); i++) {
            fat->writeCopy(f16bs->getFatOffset(i));
        }
        
        f16bs->write();
        
        auto fs = akaifat::fat::AkaiFatFileSystem::read(&device, false);
        
        if (!label.empty())
            fs->setVolumeLabel(label);
        
        fs->flush();
        return fs;
    }
    
    void setFatType(std::shared_ptr<akaifat::fat::FatType> _fatType) {
        reservedSectors = 1;
        sectorsPerCluster = sectorsPerCluster16();
        fatType = _fatType;
    }
    
    SuperFloppyFormatter(BlockDevice& _device)
    : device (_device), oemName (DEFAULT_OEM_NAME()),
    fatCount (DEFAULT_FAT_COUNT) {
        setFatType(std::make_shared<akaifat::fat::Fat16Type>());
    }
};
}
