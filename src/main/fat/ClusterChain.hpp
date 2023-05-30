#pragma once

#include <algorithm>
#include <cstdint>

#include "../AbstractFsObject.hpp"

#include "Fat.hpp"

namespace akaifat::fat {
    class ClusterChain : public akaifat::AbstractFsObject {
    private:
        Fat *fat;
        std::shared_ptr<BlockDevice> device;
        std::int32_t clusterSize;
        std::int64_t dataOffset;

        std::int64_t startCluster;

        std::int64_t getDevOffset(std::int64_t cluster, std::int32_t clusterOffset) {
            return dataOffset + clusterOffset +
                   ((cluster - Fat::FIRST_CLUSTER) * clusterSize);
        }

    public:
        ClusterChain(Fat *fat, bool readOnly)
                : ClusterChain(fat, 0, readOnly) {}

        ClusterChain(Fat *_fat, std::int64_t _startCluster, bool readOnly)
                : akaifat::AbstractFsObject(readOnly), fat(_fat) {
            if (_startCluster != 0) {
                fat->testCluster(_startCluster);

                if (fat->isFreeCluster(_startCluster))
                {
                    throw std::runtime_error("cluster " + std::to_string(_startCluster) + " is free");
                }
            }

            device = fat->getDevice();
            dataOffset = fat->getBootSector()->getFilesOffset();
            startCluster = _startCluster;
            clusterSize = fat->getBootSector()->getBytesPerCluster();
        }

        std::int32_t getClusterSize() {
            return clusterSize;
        }

        Fat *getFat() {
            return fat;
        }

        std::shared_ptr<BlockDevice> getDevice() {
            return device;
        }

        std::int64_t getStartCluster() {
            return startCluster;
        }

        std::int64_t getLengthOnDisk() {
            if (getStartCluster() == 0) return 0;

            return getChainLength() * clusterSize;
        }

        std::int64_t setSize(std::int64_t size) {
            std::int64_t nrClusters = ((size + clusterSize - 1) / clusterSize);
            if (nrClusters > INT_MAX)
                throw std::runtime_error("too many clusters");

            setChainLength((std::int32_t) nrClusters);

            return clusterSize * nrClusters;
        }

        std::int32_t getChainLength() {
            if (getStartCluster() == 0) return 0;
            auto chain = getFat()->getChain(getStartCluster());
            return chain.size();
        }

        void setChainLength(std::int32_t nrClusters) {
            if (nrClusters < 0) throw std::runtime_error("negative cluster count");

            if ((startCluster == 0) && (nrClusters == 0)) {
                /* nothing to do */
            } else if ((startCluster == 0) && (nrClusters > 0)) {
                auto chain = fat->allocNew(nrClusters);
                startCluster = chain[0];
            } else {
                auto chain = fat->getChain(startCluster);

                if (nrClusters != chain.size()) {
                    if (nrClusters > chain.size()) {
                        /* grow the chain */
                        std::int32_t count = nrClusters - chain.size();

                        while (count > 0) {
                            fat->allocAppend(getStartCluster());
                            count--;
                        }
                    } else {
                        /* shrink the chain */
                        if (nrClusters > 0) {
                            fat->setEof(chain[nrClusters - 1]);
                            for (std::int32_t i = nrClusters; i < chain.size(); i++) {
                                fat->setFree(chain[i]);
                            }
                        } else {
                            for (std::int64_t i : chain) {
                                fat->setFree(i);
                            }

                            startCluster = 0;
                        }
                    }
                }
            }
        }

        void readData(std::int64_t offset, ByteBuffer &dest) {

            std::int32_t len = (std::int32_t) dest.remaining();

            if (startCluster == 0 && len > 0) {
                throw std::runtime_error("cannot read from empty cluster chain");
            }

            auto chain = getFat()->getChain(startCluster);
            auto dev = getDevice();

            auto chainIdx = (std::int32_t) (offset / clusterSize);

            if (offset % clusterSize != 0) {
                auto clusOfs = (std::int32_t) (offset % clusterSize);
                std::int32_t size = std::min<int>(len,  clusterSize - (offset % clusterSize));
                dest.limit(dest.position() + size);

                dev->read(getDevOffset(chain[chainIdx], clusOfs), dest);

                len -= size;
                chainIdx++;
            }

            while (len > 0) {
                std::int32_t size = std::min<int>(clusterSize, len);
                dest.limit(dest.position() + size);

                dev->read(getDevOffset(chain[chainIdx], 0), dest);

                len -= size;
                chainIdx++;
            }
        }

        void writeData(std::int64_t offset, ByteBuffer &srcBuf) {

            std::int32_t len = srcBuf.remaining();

            if (len == 0) return;

            std::int64_t minSize = offset + len;
            if (getLengthOnDisk() < minSize) {
                setSize(minSize);
            }

            auto chain = fat->getChain(getStartCluster());

            auto chainIdx = (std::int32_t) (offset / clusterSize);

            if (offset % clusterSize != 0) {
                auto clusOfs = (std::int32_t) (offset % clusterSize);
                std::int32_t size = std::min<int>(len,
                                    (std::int32_t) (clusterSize - (offset % clusterSize)));
                srcBuf.limit(srcBuf.position() + size);

                device->write(getDevOffset(chain[chainIdx], clusOfs), srcBuf);

                len -= size;
                chainIdx++;
            }

            while (len > 0) {
                std::int32_t size = std::min<int>(clusterSize, len);
                srcBuf.limit(srcBuf.position() + size);

                device->write(getDevOffset(chain[chainIdx], 0), srcBuf);

                len -= size;
                chainIdx++;
            }

        }
    };
}
