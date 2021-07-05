#pragma once

#include "Sector.hpp"

#include "FatType.hpp"

namespace akaifat::fat {
    class BootSector : public Sector {
    public:
        static const int FAT_COUNT_OFFSET = 16;
        static const int RESERVED_SECTORS_OFFSET = 14;
        static const int TOTAL_SECTORS_16_OFFSET = 19;
        static const int TOTAL_SECTORS_32_OFFSET = 32;
        static const int FILE_SYSTEM_TYPE_LENGTH = 8;
        static const int SECTORS_PER_CLUSTER_OFFSET = 0x0d;
        static const int EXTENDED_BOOT_SIGNATURE = 0x29;
        static const int SIZE = 512;

        static std::shared_ptr<BootSector> read(BlockDevice *device);

        virtual FatType *getFatType() = 0;

        virtual long getSectorsPerFat() = 0;

        virtual void setSectorsPerFat(long v) = 0;

        virtual void setSectorCount(long count) = 0;

        virtual int getRootDirEntryCount() = 0;

        virtual long getSectorCount() = 0;

        int getBytesPerSector() {
            return get16(0x0b);
        }

        int getNrReservedSectors() {
            return get16(RESERVED_SECTORS_OFFSET);
        }

        long getFatOffset(int fatNr) {
            long sectSize = getBytesPerSector();
            long sectsPerFat = getSectorsPerFat();
            long resSects = getNrReservedSectors();

            long offset = resSects * sectSize;
            long fatSize = sectsPerFat * sectSize;

            offset += fatNr * fatSize;

            return offset;
        }

        int getNrFats() {
            return get8(FAT_COUNT_OFFSET);
        }

        long getRootDirOffset() {
            long sectSize = getBytesPerSector();
            long sectsPerFat = getSectorsPerFat();
            int fats = getNrFats();

            long offset = getFatOffset(0);

            offset += fats * sectsPerFat * sectSize;

            return offset;
        }

        long getFilesOffset() {
            long offset = getRootDirOffset();

            offset += getRootDirEntryCount() * 32l;

            return offset;
        }

        virtual int getFileSystemTypeLabelOffset() = 0;

        virtual int getExtendedBootSignatureOffset() = 0;

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

            for (int i = 0; i < FILE_SYSTEM_TYPE_LENGTH; i++) {
                result += ((char) get8(getFileSystemTypeLabelOffset() + i));
            }

            return result;
        }

        void setFileSystemTypeLabel(std::string &fsType) {

            if (fsType.length() != FILE_SYSTEM_TYPE_LENGTH) {
                throw std::runtime_error("invalid file system type length");
            }

            for (int i = 0; i < FILE_SYSTEM_TYPE_LENGTH; i++) {
                set8(getFileSystemTypeLabelOffset() + i, fsType[i]);
            }
        }

        long getDataClusterCount() {
            return getDataSize() / getBytesPerCluster();
        }

        std::string getOemName() {
            std::string result;

            for (int i = 0; i < 8; i++) {
                int v = get8(0x3 + i);
                if (v == 0) break;
                result += (char) v;
            }

            return result;
        }

        void setOemName(std::string &name) {
            if (name.length() > 8)
                throw std::runtime_error("only 8 characters are allowed");

            for (int i = 0; i < 8; i++) {
                char ch;
                if (i < name.length()) {
                    ch = name[i];
                } else {
                    ch = (char) 0;
                }

                set8(0x3 + i, ch);
            }
        }

        void setBytesPerSector(int v) {
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

        int getBytesPerCluster() {
            return getSectorsPerCluster() * getBytesPerSector();
        }

        int getSectorsPerCluster() {
            return get8(SECTORS_PER_CLUSTER_OFFSET);
        }

        void setSectorsPerCluster(int v) {
            if (v == getSectorsPerCluster()) return;
            if (!isPowerOfTwo(v)) throw std::runtime_error("value must be a power of two");

            set8(SECTORS_PER_CLUSTER_OFFSET, v);
        }

        void setNrReservedSectors(int v) {
            if (v == getNrReservedSectors()) return;
            if (v < 1) throw std::runtime_error("there must be >= 1 reserved sectors");
            set16(RESERVED_SECTORS_OFFSET, v);
        }

        void setNrFats(int v) {
            if (v == getNrFats()) return;

            set8(FAT_COUNT_OFFSET, v);
        }

        int getMediumDescriptor() {
            return get8(0x15);
        }

        void setMediumDescriptor(int v) {
            set8(0x15, v);
        }

        int getSectorsPerTrack() {
            return get16(0x18);
        }


        void setSectorsPerTrack(int v) {
            if (v == getSectorsPerTrack()) return;

            set16(0x18, v);
        }

        int getNrHeads() {
            return get16(0x1a);
        }

        void setNrHeads(int v) {
            if (v == getNrHeads()) return;

            set16(0x1a, v);
        }

        long getNrHiddenSectors() {
            return get32(0x1c);
        }

        void setNrHiddenSectors(long v) {
            if (v == getNrHiddenSectors()) return;

//            set32(0x1c, v);
        }

    protected:
        explicit BootSector(BlockDevice *device)
                : Sector(device, 0, SIZE) {
            markDirty();
        }

        int getNrLogicalSectors() {
            return get16(TOTAL_SECTORS_16_OFFSET);
        }

        void setNrLogicalSectors(int v) {
            if (v == getNrLogicalSectors()) return;

            set16(TOTAL_SECTORS_16_OFFSET, v);
        }

        void setNrTotalSectors(long v) {
            set32(TOTAL_SECTORS_32_OFFSET, v);
        }

        long getNrTotalSectors() {
            return get32(TOTAL_SECTORS_32_OFFSET);
        }

    private:
        static bool isPowerOfTwo(int n) {
            return ((n != 0) && (n & (n - 1)) == 0);
        }

        long getDataSize() {
            return (getSectorCount() * getBytesPerSector()) - getFilesOffset();
        }
    };
}
