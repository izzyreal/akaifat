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
            std::int64_t trueSize = chain->getLengthOnDisk();

            if (trueSize > toWrite) {
                auto rest = (std::int32_t) (trueSize - toWrite);
                ByteBuffer fill(rest);
                chain->writeData(toWrite, fill);
            }
        }

        void changeSize(std::int32_t entryCount) override {

            assert (entryCount >= 0);

            std::int32_t size = entryCount * 32;

            if (size > MAX_SIZE)
                throw std::runtime_error("directory would grow beyond " + std::to_string(MAX_SIZE) + " bytes");

            sizeChanged(chain->setSize(std::max<std::int64_t>(size, chain->getClusterSize())));
        }

    public:
        static const std::int32_t MAX_SIZE = 65536 * 32;

        std::shared_ptr<ClusterChain> chain;

        ClusterChainDirectory(const std::shared_ptr<ClusterChain>& _chain, bool isRoot)
                : AbstractDirectory(
                (std::int32_t) (_chain->getLengthOnDisk() / 32),
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

        std::int64_t getStorageCluster() override {
            return isRoot() ? 0 : chain->getStartCluster();
        }

        void read(ByteBuffer &data) override {
            chain->readData(0, data);
        }
    };
}
