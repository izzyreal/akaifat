#pragma once

#include "BootSector.hpp"
#include "FatType.hpp"

#include <memory>
#include <utility>

namespace akaifat::fat {
class Fat {
private:
    std::vector<long> entries;
    FatType* fatType;
    std::shared_ptr<BootSector> bs;
    long offset;
    int lastClusterIndex;
    int sectorCount;
    int sectorSize;
    BlockDevice* device;
    
    int lastAllocatedCluster;

    void init(int mediumDescriptor) {
        entries[0] =
                (mediumDescriptor & 0xFF) |
                (0xFFFFF00L & fatType->getBitMask());
        entries[1] = fatType->getEofMarker();
    }
    
    void read() {
        ByteBuffer bb(sectorCount * sectorSize);
        device->read(offset, bb);

        for (int i = 0; i < entries.size(); i++)
            entries[i] = fatType->readEntry(bb.getBuffer(), i);
    }
    
public:
    static const int FIRST_CLUSTER = 2;

    Fat(std::shared_ptr<BootSector> _bs, long _offset)
            : bs (std::move(_bs)), offset (_offset)
    {
        device = bs->getDevice();
        fatType = bs->getFatType();

        if (bs->getSectorsPerFat() > INT_MAX)
            throw std::runtime_error("FAT too large");

        if (bs->getSectorsPerFat() <= 0)
            throw std::runtime_error("boot sector says there are " + std::to_string(bs->getSectorsPerFat()) +
                                     " sectors per FAT");

        if (bs->getBytesPerSector() <= 0)
            throw std::runtime_error("boot sector says there are " + std::to_string(bs->getBytesPerSector()) +
                                     " bytes per sector");

        sectorCount = (int) bs->getSectorsPerFat();
        sectorSize = bs->getBytesPerSector();
        lastAllocatedCluster = FIRST_CLUSTER;

        if (bs->getDataClusterCount() > INT_MAX) throw
                    std::runtime_error("too many data clusters");

        if (bs->getDataClusterCount() == 0) throw
                    std::runtime_error("no data clusters");

        lastClusterIndex = (int) bs->getDataClusterCount() + FIRST_CLUSTER;

        entries = std::vector<long>((sectorCount * sectorSize) /
                                    fatType->getEntrySize());

        if (lastClusterIndex > entries.size())
            throw std::runtime_error("file system has " + std::to_string(lastClusterIndex) +
                                     " clusters but only " + std::to_string(entries.size()) + " FAT entries");
    }

    static std::shared_ptr<Fat> read(std::shared_ptr<BootSector> bs, int fatNr) {
        
        if (fatNr > bs->getNrFats()) {
            throw std::runtime_error("boot sector says there are only " + std::to_string(bs->getNrFats()) +
                    " FATs when reading FAT #" + std::to_string(fatNr));
        }
        
        long fatOffset = bs->getFatOffset(fatNr);
        auto result = std::make_shared<Fat>(bs, fatOffset);
        result->read();
        return result;
    }
    
    static std::shared_ptr<Fat> create(std::shared_ptr<BootSector> bs, int fatNr) {
        
        if (fatNr > bs->getNrFats()) {
            throw std::runtime_error("boot sector says there are only " + std::to_string(bs->getNrFats()) +
                    " FATs when creating FAT #" + std::to_string(fatNr));
        }
        
        long fatOffset = bs->getFatOffset(fatNr);
        auto result = std::make_shared<Fat>(bs, fatOffset);

        if (bs->getDataClusterCount() > result->entries.size())
            throw std::runtime_error("FAT too small for device");
            
        result->init(bs->getMediumDescriptor());
        result->write();
        return result;
    }

    FatType* getFatType() {
        return fatType;
    }
    
    std::shared_ptr<BootSector> getBootSector() {
        return bs;
    }

    BlockDevice* getDevice() {
        return device;
    }
   
    void write() {
        writeCopy(offset);
    }
    
    void writeCopy(long _offset) {
        std::vector<char> data(sectorCount * sectorSize);
        
        for (int index = 0; index < entries.size(); index++) {
            fatType->writeEntry(data, index, entries[index]);
        }
        
        auto bb = ByteBuffer(data);
        device->write(_offset, bb);
    }
    
    int getMediumDescriptor() {
        return (int) (entries[0] & 0xFF);
    }
    
    long getEntry(int index) {
        return entries[index];
    }

    int getLastFreeCluster() {
        return lastAllocatedCluster;
    }
    
    std::vector<long> getChain(long startCluster) {
        testCluster(startCluster);
        // Count the chain first
        int count = 1;
        long cluster = startCluster;
        while (!isEofCluster(entries[(int) cluster])) {
            count++;
            cluster = entries[(int) cluster];
        }
        // Now create the chain
        std::vector<long> chain(count);
        chain[0] = startCluster;
        cluster = startCluster;
        int i = 0;
        while (!isEofCluster(entries[(int) cluster])) {
            cluster = entries[(int) cluster];
            chain[++i] = cluster;
        }
        return chain;
    }

    long getNextCluster(long cluster) {
        testCluster(cluster);
        long entry = entries[(int) cluster];
        if (isEofCluster(entry)) {
            return -1;
        } else {
            return entry;
        }
    }

    long allocNew() {

        int i;
        int entryIndex = -1;

        for (i = lastAllocatedCluster; i < lastClusterIndex; i++) {
            if (isFreeCluster(i)) {
                entryIndex = i;
                break;
            }
        }
        
        if (entryIndex < 0) {
            for (i = FIRST_CLUSTER; i < lastAllocatedCluster; i++) {
                if (isFreeCluster(i)) {
                    entryIndex = i;
                    break;
                }
            }
        }
        
        if (entryIndex < 0) {
            throw std::runtime_error("FAT Full (" + std::to_string(lastClusterIndex - FIRST_CLUSTER)
                    + ", " + std::to_string(i) + ")");
        }
        
        entries[entryIndex] = fatType->getEofMarker();
        lastAllocatedCluster = entryIndex % lastClusterIndex;
        if (lastAllocatedCluster < FIRST_CLUSTER)
            lastAllocatedCluster = FIRST_CLUSTER;
        
        return entryIndex;
    }
    
    int getFreeClusterCount() {
        int result = 0;

        for (int i=FIRST_CLUSTER; i < lastClusterIndex; i++) {
            if (isFreeCluster(i)) result++;
        }

        return result;
    }

    int getLastAllocatedCluster() {
        return lastAllocatedCluster;
    }
    
    std::vector<long> allocNew(int nrClusters) {
        std::vector<long> rc(nrClusters);
        
        rc[0] = allocNew();
        for (int i = 1; i < nrClusters; i++) {
            rc[i] = allocAppend(rc[i - 1]);
        }
        
        return rc;
    }
    
    long allocAppend(long cluster) {
        
        testCluster(cluster);
        
        while (!isEofCluster(entries[(int) cluster])) {
            cluster = entries[(int) cluster];
        }
        
        long newCluster = allocNew();
        entries[(int) cluster] = newCluster;

        return newCluster;
    }

    void setEof(long cluster) {
        testCluster(cluster);
        entries[(int) cluster] = fatType->getEofMarker();
    }

    void setFree(long cluster) {
        testCluster(cluster);
        entries[(int) cluster] = 0;
    }
    
    bool equals(const std::shared_ptr<Fat>& other) {
        if (fatType != other->fatType) return false;
        if (sectorCount != other->sectorCount) return false;
        if (sectorSize != other->sectorSize) return false;
        if (lastClusterIndex != other->lastClusterIndex) return false;

        if (entries.size() != other->entries.size()) return false;

        for (int i = 0; i < entries.size(); i++)
            if (entries[i] != other->entries[i]) return false;

        return (getMediumDescriptor() == other->getMediumDescriptor());
    }
    
    int hashCode() {
        int hash = 7;
//        hash = 23 * hash + Arrays.hashCode(entries);
        hash = 23 * hash + (int) fatType->hashCode();
        hash = 23 * hash + sectorCount;
        hash = 23 * hash + sectorSize;
        hash = 23 * hash + lastClusterIndex;
        return hash;
    }

    // Can be protected?
    void testCluster(long cluster) {
        if ((cluster < FIRST_CLUSTER) || (cluster >= entries.size())) {
            throw std::runtime_error("invalid cluster value " + std::to_string(cluster));
        }
    }

    bool isFreeCluster(long entry) {
        if (entry > INT_MAX) throw std::runtime_error("entry is bigger than INT_MAX");
        return (entries[(int) entry] == 0);
    }
    
    bool isReservedCluster(long entry) {
        return fatType->isReservedCluster(entry);
    }

    bool isEofCluster(long entry) {
        return fatType->isEofCluster(entry);
    }

};
}
