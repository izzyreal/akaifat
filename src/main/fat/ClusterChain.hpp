#pragma once

#include "../AbstractFsObject.hpp"

#include "Fat.hpp"

#include <algorithm>

namespace akaifat::fat {
    class ClusterChain : public akaifat::AbstractFsObject {
    private:
        Fat *fat;
        BlockDevice *device;
        int clusterSize;
        long dataOffset;

        long startCluster;

        long getDevOffset(long cluster, int clusterOffset) {
            return dataOffset + clusterOffset +
                   ((cluster - Fat::FIRST_CLUSTER) * clusterSize);
        }

    public:
        ClusterChain(Fat *fat, bool readOnly)
                : ClusterChain(fat, 0, readOnly) {}

        ClusterChain(Fat *_fat, long _startCluster, bool readOnly)
                : akaifat::AbstractFsObject(readOnly), fat(_fat) {
            if (_startCluster != 0) {
                fat->testCluster(_startCluster);

                if (fat->isFreeCluster(_startCluster))
                    throw std::runtime_error("cluster " + std::to_string(_startCluster) + " is free");
            }

            device = fat->getDevice();
            dataOffset = fat->getBootSector()->getFilesOffset();
            startCluster = _startCluster;
            clusterSize = fat->getBootSector()->getBytesPerCluster();
        }

        int getClusterSize() {
            return clusterSize;
        }

        Fat *getFat() {
            return fat;
        }

        BlockDevice *getDevice() {
            return device;
        }

        long getStartCluster() {
            return startCluster;
        }

        long getLengthOnDisk() {
            if (getStartCluster() == 0) return 0;

            return getChainLength() * clusterSize;
        }

        long setSize(long size) {
            long nrClusters = ((size + clusterSize - 1) / clusterSize);
            if (nrClusters > INT_MAX)
                throw std::runtime_error("too many clusters");

            setChainLength((int) nrClusters);

            return clusterSize * nrClusters;
        }

        int getChainLength() {
            if (getStartCluster() == 0) return 0;
            auto chain = getFat()->getChain(getStartCluster());
            return chain.size();
        }

        void setChainLength(int nrClusters) {
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
                        int count = nrClusters - chain.size();

                        while (count > 0) {
                            fat->allocAppend(getStartCluster());
                            count--;
                        }
                    } else {
                        /* shrink the chain */
                        if (nrClusters > 0) {
                            fat->setEof(chain[nrClusters - 1]);
                            for (int i = nrClusters; i < chain.size(); i++) {
                                fat->setFree(chain[i]);
                            }
                        } else {
                            for (long i : chain) {
                                fat->setFree(i);
                            }

                            startCluster = 0;
                        }
                    }
                }
            }
        }

        void readData(long offset, ByteBuffer &dest) {

            int len = (int) dest.remaining();

            if ((startCluster == 0 && len > 0)) {
                throw std::runtime_error("cannot read from empty cluster chain");
            }

            auto chain = getFat()->getChain(startCluster);
            auto dev = getDevice();

            int chainIdx = (int) (offset / clusterSize);

            if (offset % clusterSize != 0) {
                int clusOfs = (int) (offset % clusterSize);
                int size = std::min(len,
                                    (int) (clusterSize - (offset % clusterSize)));
                dest.limit(dest.position() + size);

                dev->read(getDevOffset(chain[chainIdx], clusOfs), dest);

                len -= size;
                chainIdx++;
            }

            while (len > 0) {
                int size = std::min(clusterSize, len);
                dest.limit(dest.position() + size);

                dev->read(getDevOffset(chain[chainIdx], 0), dest);

                len -= size;
                chainIdx++;
            }
        }

        void writeData(long offset, ByteBuffer &srcBuf) {

            int len = srcBuf.remaining();

            if (len == 0) return;

            long minSize = offset + len;
            if (getLengthOnDisk() < minSize) {
                setSize(minSize);
            }

            auto chain = fat->getChain(getStartCluster());

            int chainIdx = (int) (offset / clusterSize);

            if (offset % clusterSize != 0) {
                int clusOfs = (int) (offset % clusterSize);
                int size = std::min(len,
                                    (int) (clusterSize - (offset % clusterSize)));
                srcBuf.limit(srcBuf.position() + size);

                device->write(getDevOffset(chain[chainIdx], clusOfs), srcBuf);

                len -= size;
                chainIdx++;
            }

            while (len > 0) {
                int size = std::min(clusterSize, len);
                srcBuf.limit(srcBuf.position() + size);

                device->write(getDevOffset(chain[chainIdx], 0), srcBuf);

                len -= size;
                chainIdx++;
            }

        }
    };
}
