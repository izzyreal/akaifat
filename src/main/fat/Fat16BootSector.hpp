#pragma once

#include "BootSector.hpp"
#include "FatType.hpp"

#include <string>
#include <utility>

namespace akaifat::fat {
    class Fat16BootSector : public BootSector {
    public:
        static const std::int32_t DEFAULT_ROOT_DIR_ENTRY_COUNT = 512;

        static std::string &DEFAULT_VOLUME_LABEL() {
            static std::string result = "NO NAME";
            return result;
        }

        static const std::int32_t MAX_FAT16_CLUSTERS = 65524;
        static const std::int32_t SECTORS_PER_FAT_OFFSET = 0x16;
        static const std::int32_t ROOT_DIR_ENTRIES_OFFSET = 0x11;
        static const std::int32_t VOLUME_LABEL_OFFSET = 0x2b;
        static const std::int32_t FILE_SYSTEM_TYPE_OFFSET = 0x36;
        static const std::int32_t MAX_VOLUME_LABEL_LENGTH = 11;
        static const std::int32_t EXTENDED_BOOT_SIGNATURE_OFFSET = 0x26;

        explicit Fat16BootSector(std::shared_ptr<BlockDevice> device)
                : BootSector(std::move(device)) {
        }

        std::string getVolumeLabel() {
            std::string result;

            for (std::int32_t i = 0; i < MAX_VOLUME_LABEL_LENGTH; i++) {
                char c = (char) get8(VOLUME_LABEL_OFFSET + i);

                if (c != 0) {
                    result += c;
                } else {
                    break;
                }
            }

            return result;
        }

        void setVolumeLabel(std::string label) {
            if (label.length() > MAX_VOLUME_LABEL_LENGTH)
                throw std::runtime_error("volume label too std::int64_t");

            for (std::int32_t i = 0; i < MAX_VOLUME_LABEL_LENGTH; i++) {
                set8(VOLUME_LABEL_OFFSET + i,
                     i < label.length() ? label[i] : 0);
            }
        }


        std::int64_t getSectorsPerFat() override {
            return get16(SECTORS_PER_FAT_OFFSET);
        }


        void setSectorsPerFat(std::int64_t v) override {
            if (v == getSectorsPerFat()) return;
            if (v > 0x7FFF) throw std::runtime_error("too many sectors for a FAT12/16");

            set16(SECTORS_PER_FAT_OFFSET, (std::int32_t) v);
        }


        FatType *getFatType() override {
            auto rootDirEntryCount = getRootDirEntryCount();
            auto bytesPerSector = getBytesPerSector();

            std::int64_t rootDirSectors = ((rootDirEntryCount * 32) +
                                   (bytesPerSector - 1)) / bytesPerSector;

            auto sectorCount = getSectorCount();
            auto nrReservedSectors = getNrReservedSectors();
            auto nrFats = getNrFats();
            auto sectorsPerFat = getSectorsPerFat();

            std::int64_t dataSectors = sectorCount -
                               (nrReservedSectors + (nrFats * sectorsPerFat) +
                                rootDirSectors);

            auto sectorsPerCluster = getSectorsPerCluster();
            std::int64_t clusterCount = dataSectors / sectorsPerCluster;

            if (clusterCount > MAX_FAT16_CLUSTERS)
                throw std::runtime_error("too many clusters for FAT16: " + std::to_string(clusterCount));

            static auto result = new Fat16Type();

            return result;
        }


        void setSectorCount(std::int64_t count) override {
            if (count > 65535) {
                setNrLogicalSectors(0);
                setNrTotalSectors(count);
            } else {
                setNrLogicalSectors((std::int32_t) count);
                setNrTotalSectors(count);
            }
        }


        std::int64_t getSectorCount() override {
            if (getNrLogicalSectors() == 0) return getNrTotalSectors();
            else return getNrLogicalSectors();
        }


        std::int32_t getRootDirEntryCount() override {
            return get16(ROOT_DIR_ENTRIES_OFFSET);
        }

        void setRootDirEntryCount(std::int32_t v) {
            if (v < 0) throw std::runtime_error("root dir entry count has to be 0 or more");
            if (v == getRootDirEntryCount()) return;

            set16(ROOT_DIR_ENTRIES_OFFSET, v);
        }


        void init() override {
            BootSector::init();

            setRootDirEntryCount(DEFAULT_ROOT_DIR_ENTRY_COUNT);
            setVolumeLabel(DEFAULT_VOLUME_LABEL());
        }


        std::int32_t getFileSystemTypeLabelOffset() override {
            return FILE_SYSTEM_TYPE_OFFSET;
        }


        std::int32_t getExtendedBootSignatureOffset() override {
            return EXTENDED_BOOT_SIGNATURE_OFFSET;
        }

    };
}
