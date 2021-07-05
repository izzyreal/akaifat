#pragma once

#include "AbstractDirectory.hpp"
#include "ClusterChain.hpp"

#include <algorithm>

namespace akaifat::fat {
    class ClusterChainDirectory : public AbstractDirectory {

    public:
        void read() override { AbstractDirectory::read(); }

    protected:
        void write(ByteBuffer &data) override {
            auto toWrite = data.remaining();
            chain->writeData(0, data);
            long trueSize = chain->getLengthOnDisk();

            if (trueSize > toWrite) {
                int rest = (int) (trueSize - toWrite);
                ByteBuffer fill(rest);
                chain->writeData(toWrite, fill);
            }
        }

        void changeSize(int entryCount) override {

            assert (entryCount >= 0);

            int size = entryCount * 32;

            if (size > MAX_SIZE)
                throw std::runtime_error("directory would grow beyond " + std::to_string(MAX_SIZE) + " bytes");

            sizeChanged(chain->setSize(std::max(size, chain->getClusterSize())));
        }

    public:
        static const int MAX_SIZE = 65536 * 32;

        std::shared_ptr<ClusterChain> chain;

        ClusterChainDirectory(const std::shared_ptr<ClusterChain>& _chain, bool isRoot)
                : AbstractDirectory(
                (int) (_chain->getLengthOnDisk() / 32),
                _chain->isReadOnly(), isRoot), chain(_chain) {
        }

        static std::shared_ptr<ClusterChainDirectory> readRoot(
                const std::shared_ptr<ClusterChain>& chain) {

            auto result = std::make_shared<ClusterChainDirectory>(chain, true);

            result->read();
            return result;
        }

        void delete_() {
            chain->setChainLength(0);
        }

        long getStorageCluster() override {
            return isRoot() ? 0 : chain->getStartCluster();
        }

        void read(ByteBuffer &data) override {
            chain->readData(0, data);
        }
    };
}
