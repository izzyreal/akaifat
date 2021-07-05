#pragma once

#include "BootSector.hpp"
#include "FatType.hpp"

#include <string>

namespace akaifat::fat {
    class Fat16BootSector : public BootSector {
    public:
        static const int DEFAULT_ROOT_DIR_ENTRY_COUNT = 512;

        static std::string &DEFAULT_VOLUME_LABEL() {
            static std::string result = "NO NAME";
            return result;
        }

        static const int MAX_FAT16_CLUSTERS = 65524;
        static const int SECTORS_PER_FAT_OFFSET = 0x16;
        static const int ROOT_DIR_ENTRIES_OFFSET = 0x11;
        static const int VOLUME_LABEL_OFFSET = 0x2b;
        static const int FILE_SYSTEM_TYPE_OFFSET = 0x36;
        static const int MAX_VOLUME_LABEL_LENGTH = 11;
        static const int EXTENDED_BOOT_SIGNATURE_OFFSET = 0x26;

        explicit Fat16BootSector(BlockDevice *device)
                : BootSector(device) {
        }

        std::string getVolumeLabel() {
            std::string result;

            for (int i = 0; i < MAX_VOLUME_LABEL_LENGTH; i++) {
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
                throw std::runtime_error("volume label too long");

            for (int i = 0; i < MAX_VOLUME_LABEL_LENGTH; i++) {
                set8(VOLUME_LABEL_OFFSET + i,
                     i < label.length() ? label[i] : 0);
            }
        }


        long getSectorsPerFat() override {
            return get16(SECTORS_PER_FAT_OFFSET);
        }


        void setSectorsPerFat(long v) override {
            if (v == getSectorsPerFat()) return;
            if (v > 0x7FFF) throw std::runtime_error("too many sectors for a FAT12/16");

            set16(SECTORS_PER_FAT_OFFSET, (int) v);
        }


        FatType *getFatType() override {
            auto rootDirEntryCount = getRootDirEntryCount();
            auto bytesPerSector = getBytesPerSector();

            long rootDirSectors = ((rootDirEntryCount * 32) +
                                   (bytesPerSector - 1)) / bytesPerSector;

            auto sectorCount = getSectorCount();
            auto nrReservedSectors = getNrReservedSectors();
            auto nrFats = getNrFats();
            auto sectorsPerFat = getSectorsPerFat();

            long dataSectors = sectorCount -
                               (nrReservedSectors + (nrFats * sectorsPerFat) +
                                rootDirSectors);

            auto sectorsPerCluster = getSectorsPerCluster();
            long clusterCount = dataSectors / sectorsPerCluster;

            if (clusterCount > MAX_FAT16_CLUSTERS)
                throw std::runtime_error("too many clusters for FAT16: " + std::to_string(clusterCount));

            return new Fat16Type();
        }


        void setSectorCount(long count) override {
            if (count > 65535) {
                setNrLogicalSectors(0);
                setNrTotalSectors(count);
            } else {
                setNrLogicalSectors((int) count);
                setNrTotalSectors(count);
            }
        }


        long getSectorCount() override {
            if (getNrLogicalSectors() == 0) return getNrTotalSectors();
            else return getNrLogicalSectors();
        }


        int getRootDirEntryCount() override {
            return get16(ROOT_DIR_ENTRIES_OFFSET);
        }

        void setRootDirEntryCount(int v) {
            if (v < 0) throw std::runtime_error("root dir entry count has to be 0 or more");
            if (v == getRootDirEntryCount()) return;

            set16(ROOT_DIR_ENTRIES_OFFSET, v);
        }


        void init() override {
            BootSector::init();

            setRootDirEntryCount(DEFAULT_ROOT_DIR_ENTRY_COUNT);
            setVolumeLabel(DEFAULT_VOLUME_LABEL());
        }


        int getFileSystemTypeLabelOffset() override {
            return FILE_SYSTEM_TYPE_OFFSET;
        }


        int getExtendedBootSignatureOffset() override {
            return EXTENDED_BOOT_SIGNATURE_OFFSET;
        }

    };
}
