#pragma once

#include "Sector.hpp"

#include <utility>

#include "FatType.hpp"

namespace akaifat::fat {
    class BootSector : public Sector {
    public:
        static const std::int32_t FAT_COUNT_OFFSET = 16;
        static const std::int32_t RESERVED_SECTORS_OFFSET = 14;
        static const std::int32_t TOTAL_SECTORS_16_OFFSET = 19;
        static const std::int32_t TOTAL_SECTORS_32_OFFSET = 32;
        static const std::int32_t FILE_SYSTEM_TYPE_LENGTH = 8;
        static const std::int32_t SECTORS_PER_CLUSTER_OFFSET = 0x0d;
        static const std::int32_t EXTENDED_BOOT_SIGNATURE = 0x29;
        static const std::int32_t SIZE = 512;

        static std::shared_ptr<BootSector> read(const std::shared_ptr<BlockDevice>& device);

        virtual FatType *getFatType() = 0;

        virtual std::int64_t getSectorsPerFat() = 0;

        virtual void setSectorsPerFat(std::int64_t v) = 0;

        virtual void setSectorCount(std::int64_t count) = 0;

        virtual std::int32_t getRootDirEntryCount() = 0;

        virtual std::int64_t getSectorCount() = 0;

        std::int32_t getBytesPerSector() {
            return get16(0x0b);
        }

        std::int32_t getNrReservedSectors() {
            return get16(RESERVED_SECTORS_OFFSET);
        }

        std::int64_t getFatOffset(std::int32_t fatNr) {
            std::int64_t sectSize = getBytesPerSector();
            std::int64_t sectsPerFat = getSectorsPerFat();
            std::int64_t resSects = getNrReservedSectors();

            std::int64_t offset = resSects * sectSize;
            std::int64_t fatSize = sectsPerFat * sectSize;

            offset += fatNr * fatSize;

            return offset;
        }

        std::int32_t getNrFats() {
            return get8(FAT_COUNT_OFFSET);
        }

        std::int64_t getRootDirOffset() {
            std::int64_t sectSize = getBytesPerSector();
            std::int64_t sectsPerFat = getSectorsPerFat();
            std::int32_t fats = getNrFats();

            std::int64_t offset = getFatOffset(0);

            offset += fats * sectsPerFat * sectSize;

            return offset;
        }

        std::int64_t getFilesOffset() {
            std::int64_t offset = getRootDirOffset();

            offset += getRootDirEntryCount() * 32l;

            return offset;
        }

        virtual std::int32_t getFileSystemTypeLabelOffset() = 0;

        virtual std::int32_t getExtendedBootSignatureOffset() = 0;

        virtual void init() {
            setBytesPerSector(getDevice()->getSectorSize());
            setSectorCount(getDevice()->getSize() / getDevice()->getSectorSize());
            set8(getExtendedBootSignatureOffset(), EXTENDED_BOOT_SIGNATURE);

            set8(0x00, 0xeb);
            set8(0x01, 0x3c);
            set8(0x02, 0x90);
            set8(0x1fe, 0x55);
            set8(0x1ff, 0xaa);
        }

        std::string getFileSystemTypeLabel() {
            std::string result;

            for (std::int32_t i = 0; i < FILE_SYSTEM_TYPE_LENGTH; i++) {
                result += ((char) get8(getFileSystemTypeLabelOffset() + i));
            }

            return result;
        }

        void setFileSystemTypeLabel(std::string &fsType) {

            if (fsType.length() != FILE_SYSTEM_TYPE_LENGTH) {
                throw std::runtime_error("invalid file system type length");
            }

            for (std::int32_t i = 0; i < FILE_SYSTEM_TYPE_LENGTH; i++) {
                set8(getFileSystemTypeLabelOffset() + i, fsType[i]);
            }
        }

        std::int64_t getDataClusterCount() {
            return getDataSize() / getBytesPerCluster();
        }

        std::string getOemName() {
            std::string result;

            for (std::int32_t i = 0; i < 8; i++) {
                std::int32_t v = get8(0x3 + i);
                if (v == 0) break;
                result += (char) v;
            }

            return result;
        }

        void setOemName(std::string &name) {
            if (name.length() > 8)
                throw std::runtime_error("only 8 characters are allowed");

            for (std::int32_t i = 0; i < 8; i++) {
                char ch;
                if (i < name.length()) {
                    ch = name[i];
                } else {
                    ch = (char) 0;
                }

                set8(0x3 + i, ch);
            }
        }

        void setBytesPerSector(std::int32_t v) {
            if (v == getBytesPerSector()) return;

            switch (v) {
                case 512:
                case 1024:
                case 2048:
                case 4096:
                    set16(0x0b, v);
                    break;

                default:
                    throw std::runtime_error("invalid bytes per sector");
            }
        }

        std::int32_t getBytesPerCluster() {
            return getSectorsPerCluster() * getBytesPerSector();
        }

        std::int32_t getSectorsPerCluster() {
            return get8(SECTORS_PER_CLUSTER_OFFSET);
        }

        void setSectorsPerCluster(std::int32_t v) {
            if (v == getSectorsPerCluster()) return;
            if (!isPowerOfTwo(v)) throw std::runtime_error("value must be a power of two");

            set8(SECTORS_PER_CLUSTER_OFFSET, v);
        }

        void setNrReservedSectors(std::int32_t v) {
            if (v == getNrReservedSectors()) return;
            if (v < 1) throw std::runtime_error("there must be >= 1 reserved sectors");
            set16(RESERVED_SECTORS_OFFSET, v);
        }

        void setNrFats(std::int32_t v) {
            if (v == getNrFats()) return;

            set8(FAT_COUNT_OFFSET, v);
        }

        std::int32_t getMediumDescriptor() {
            return get8(0x15);
        }

        void setMediumDescriptor(std::int32_t v) {
            set8(0x15, v);
        }

        std::int32_t getSectorsPerTrack() {
            return get16(0x18);
        }


        void setSectorsPerTrack(std::int32_t v) {
            if (v == getSectorsPerTrack()) return;

            set16(0x18, v);
        }

        std::int32_t getNrHeads() {
            return get16(0x1a);
        }

        void setNrHeads(std::int32_t v) {
            if (v == getNrHeads()) return;

            set16(0x1a, v);
        }

        std::int64_t getNrHiddenSectors() {
            return get32(0x1c);
        }

    protected:
        explicit BootSector(std::shared_ptr<BlockDevice> device)
                : Sector(std::move(device), 0, SIZE) {
            markDirty();
        }

        std::int32_t getNrLogicalSectors() {
            return get16(TOTAL_SECTORS_16_OFFSET);
        }

        void setNrLogicalSectors(std::int32_t v) {
            if (v == getNrLogicalSectors()) return;

            set16(TOTAL_SECTORS_16_OFFSET, v);
        }

        void setNrTotalSectors(std::int64_t v) {
            set32(TOTAL_SECTORS_32_OFFSET, v);
        }

        std::int64_t getNrTotalSectors() {
            return get32(TOTAL_SECTORS_32_OFFSET);
        }

    private:
        static bool isPowerOfTwo(std::int32_t n) {
            return ((n != 0) && (n & (n - 1)) == 0);
        }

        std::int64_t getDataSize() {
            return (getSectorCount() * getBytesPerSector()) - getFilesOffset();
        }
    };
}
