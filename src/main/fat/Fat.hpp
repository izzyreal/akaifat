#pragma once

#include "BootSector.hpp"
#include "FatType.hpp"

#include <memory>
#include <utility>

namespace akaifat::fat {
class Fat {
private:
    std::vector<std::int64_t> entries;
    FatType* fatType;
    std::shared_ptr<BootSector> bs;
    std::int64_t offset;
    std::int32_t lastClusterIndex;
    std::int32_t sectorCount;
    std::int32_t sectorSize;
    std::shared_ptr<BlockDevice> device;
    
    std::int32_t lastAllocatedCluster;

    void init(std::int32_t mediumDescriptor) {
        entries[0] =
                (mediumDescriptor & 0xFF) |
                (std::int64_t(0xFFFFF00) & fatType->getBitMask());
        entries[1] = fatType->getEofMarker();
    }
    
    void read() {
        ByteBuffer bb(sectorCount * sectorSize);
        device->read(offset, bb);

        for (std::int32_t i = 0; i < entries.size(); i++)
            entries[i] = fatType->readEntry(bb.getBuffer(), i);
    }
    
public:
    static const std::int32_t FIRST_CLUSTER = 2;

    Fat(std::shared_ptr<BootSector> _bs, std::int64_t _offset)
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

        sectorCount = (std::int32_t) bs->getSectorsPerFat();
        sectorSize = bs->getBytesPerSector();
        lastAllocatedCluster = FIRST_CLUSTER;

        if (bs->getDataClusterCount() > INT_MAX) throw
                    std::runtime_error("too many data clusters");

        if (bs->getDataClusterCount() == 0) throw
                    std::runtime_error("no data clusters");

        lastClusterIndex = (std::int32_t) bs->getDataClusterCount() + FIRST_CLUSTER;

        entries = std::vector<std::int64_t>((sectorCount * sectorSize) /
                                    fatType->getEntrySize());

        if (lastClusterIndex > entries.size())
            throw std::runtime_error("file system has " + std::to_string(lastClusterIndex) +
                                     " clusters but only " + std::to_string(entries.size()) + " FAT entries");
    }

    static std::shared_ptr<Fat> read(std::shared_ptr<BootSector> bs, std::int32_t fatNr) {
        
        if (fatNr > bs->getNrFats()) {
            throw std::runtime_error("boot sector says there are only " + std::to_string(bs->getNrFats()) +
                    " FATs when reading FAT #" + std::to_string(fatNr));
        }
        
        std::int64_t fatOffset = bs->getFatOffset(fatNr);
        auto result = std::make_shared<Fat>(bs, fatOffset);
        result->read();
        return result;
    }
    
    static std::shared_ptr<Fat> create(const std::shared_ptr<BootSector>& bs, std::int32_t fatNr) {
        
        if (fatNr > bs->getNrFats()) {
            throw std::runtime_error("boot sector says there are only " + std::to_string(bs->getNrFats()) +
                    " FATs when creating FAT #" + std::to_string(fatNr));
        }
        
        std::int64_t fatOffset = bs->getFatOffset(fatNr);
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

    std::shared_ptr<BlockDevice> getDevice() {
        return device;
    }
   
    void write() {
        writeCopy(offset);
    }
    
    void writeCopy(std::int64_t _offset) {
        std::vector<char> data(sectorCount * sectorSize);
        
        for (std::int32_t index = 0; index < entries.size(); index++) {
            fatType->writeEntry(data, index, entries[index]);
        }
        
        auto bb = ByteBuffer(data);
        device->write(_offset, bb);
    }
    
    std::int32_t getMediumDescriptor() {
        return (std::int32_t) (entries[0] & 0xFF);
    }
    
    std::int64_t getEntry(std::int32_t index) {
        return entries[index];
    }

    std::int32_t getLastFreeCluster() {
        return lastAllocatedCluster;
    }
    
    std::vector<std::int64_t> getChain(std::int64_t startCluster) {
        testCluster(startCluster);
        // Count the chain first
        std::int32_t count = 1;
        std::int64_t cluster = startCluster;
        while (!isEofCluster(entries[(std::int32_t) cluster])) {
            count++;
            cluster = entries[(std::int32_t) cluster];
        }
        // Now create the chain
        std::vector<std::int64_t> chain(count);
        chain[0] = startCluster;
        cluster = startCluster;
        std::int32_t i = 0;
        while (!isEofCluster(entries[(std::int32_t) cluster])) {
            cluster = entries[(std::int32_t) cluster];
            chain[++i] = cluster;
        }
        return chain;
    }

    std::int64_t getNextCluster(std::int64_t cluster) {
        testCluster(cluster);
        std::int64_t entry = entries[(std::int32_t) cluster];
        if (isEofCluster(entry)) {
            return -1;
        } else {
            return entry;
        }
    }

    std::int64_t allocNew() {

        std::int32_t i;
        std::int32_t entryIndex = -1;

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
    
    std::int32_t getFreeClusterCount() {
        std::int32_t result = 0;

        for (std::int32_t i=FIRST_CLUSTER; i < lastClusterIndex; i++) {
            if (isFreeCluster(i)) result++;
        }

        return result;
    }

    std::int32_t getLastAllocatedCluster() {
        return lastAllocatedCluster;
    }
    
    std::vector<std::int64_t> allocNew(std::int32_t nrClusters) {
        std::vector<std::int64_t> rc(nrClusters);
        
        rc[0] = allocNew();
        for (std::int32_t i = 1; i < nrClusters; i++) {
            rc[i] = allocAppend(rc[i - 1]);
        }
        
        return rc;
    }
    
    std::int64_t allocAppend(std::int64_t cluster) {
        
        testCluster(cluster);
        
        while (!isEofCluster(entries[(std::int32_t) cluster])) {
            cluster = entries[(std::int32_t) cluster];
        }
        
        std::int64_t newCluster = allocNew();
        entries[(std::int32_t) cluster] = newCluster;

        return newCluster;
    }

    void setEof(std::int64_t cluster) {
        testCluster(cluster);
        entries[(std::int32_t) cluster] = fatType->getEofMarker();
    }

    void setFree(std::int64_t cluster) {
        testCluster(cluster);
        entries[(std::int32_t) cluster] = 0;
    }
    
    bool equals(const std::shared_ptr<Fat>& other) {
        if (fatType != other->fatType) return false;
        if (sectorCount != other->sectorCount) return false;
        if (sectorSize != other->sectorSize) return false;
        if (lastClusterIndex != other->lastClusterIndex) return false;

        if (entries.size() != other->entries.size()) return false;

        for (std::int32_t i = 0; i < entries.size(); i++)
            if (entries[i] != other->entries[i]) return false;

        return (getMediumDescriptor() == other->getMediumDescriptor());
    }
    
    std::int32_t hashCode() {
        std::int32_t hash = 7;

        std::int32_t entriesHash = 1;

        for (std::int64_t element : entries) {
            auto elementHash = static_cast<std::int32_t>(element ^ ((std::uint64_t)(element) >> 32));
            entriesHash = 31 * entriesHash + elementHash;
        }

        hash = 23 * hash + entriesHash;
        hash = 23 * hash + fatType->hashCode();
        hash = 23 * hash + sectorCount;
        hash = 23 * hash + sectorSize;
        hash = 23 * hash + lastClusterIndex;
        return hash;
    }

    void testCluster(std::int64_t cluster) {
        if ((cluster < FIRST_CLUSTER) || (cluster >= entries.size())) {
            throw std::runtime_error("invalid cluster value " + std::to_string(cluster));
        }
    }

    bool isFreeCluster(std::int64_t entry) {
        if (entry > INT_MAX) throw std::runtime_error("entry is bigger than INT_MAX");
        return (entries[(std::int32_t) entry] == 0);
    }
    
    bool isReservedCluster(std::int64_t entry) {
        return fatType->isReservedCluster(entry);
    }

    bool isEofCluster(std::int64_t entry) {
        return fatType->isEofCluster(entry);
    }

};
}
