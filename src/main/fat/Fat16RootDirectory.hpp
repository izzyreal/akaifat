#pragma once

#include "AbstractDirectory.hpp"

namespace akaifat::fat {
    class Fat16RootDirectory : public AbstractDirectory {
    private:
        std::shared_ptr<BlockDevice> device;
        std::int64_t deviceOffset;

    protected:
        void read(ByteBuffer &data) override {
            device->read(deviceOffset, data);
        }

        void read() override { AbstractDirectory::read(); }

        void write(ByteBuffer &data) override {
            device->write(deviceOffset, data);
        }


        std::int64_t getStorageCluster() override {
            return 0;
        }

        void changeSize(std::int32_t entryCount) override {
            if (getCapacity() < entryCount) {
                throw std::runtime_error("directory full");
            }
        }

    public:
        Fat16RootDirectory(const std::shared_ptr<Fat16BootSector>& bs, bool readOnly)
                : AbstractDirectory(bs->getRootDirEntryCount(), readOnly, true) {
            if (bs->getRootDirEntryCount() <= 0)
                throw std::runtime_error("root directory size is " + std::to_string(bs->getRootDirEntryCount()));

            deviceOffset = bs->getRootDirOffset();
            device = bs->getDevice();
        }

        static std::shared_ptr<Fat16RootDirectory> read(
                const std::shared_ptr<Fat16BootSector>& bs, bool readOnly) {
            auto result = std::make_shared<Fat16RootDirectory>(bs, readOnly);
            result->read();
            return result;
        }

        static std::shared_ptr<Fat16RootDirectory> create(
                const std::shared_ptr<Fat16BootSector>& bs) {
            auto result = std::make_shared<Fat16RootDirectory>(bs, false);
            result->flush();
            return result;
        }
    };
}
