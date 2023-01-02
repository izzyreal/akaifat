#pragma once

#include <vector>
#include <string>

namespace akaifat::fat {
    class FatType {

    private:
        std::int64_t minReservedEntry;
        std::int64_t maxReservedEntry;
        std::int64_t eofCluster;
        std::int64_t eofMarker;
        std::int64_t bitMask;
        std::int32_t _maxClusters;
        std::string label;
        float entrySize;

    public:
        FatType(std::int32_t maxClusters,
                std::int64_t _bitMask, float _entrySize, std::string _label)
                : _maxClusters(maxClusters), bitMask(_bitMask), entrySize(_entrySize), label(std::move(_label)),
                  minReservedEntry(0xFFFFFF0L & _bitMask), maxReservedEntry(0xFFFFFF6L & _bitMask),
                  eofCluster(0xFFFFFF8L & _bitMask),
                  eofMarker(0xFFFFFFFL & _bitMask) {
        }

        virtual ~FatType() = default;

        virtual std::int64_t readEntry(std::vector<char> &data, std::int32_t index) = 0;

        virtual void writeEntry(std::vector<char> &data, std::int32_t index, std::int64_t entry) = 0;

        std::int64_t getMaxClusters() {
            return _maxClusters;
        }

        std::string getLabel() {
            return label;
        }

        bool isReservedCluster(std::int64_t entry) {
            return ((entry >= minReservedEntry) && (entry <= maxReservedEntry));
        }

        bool isEofCluster(std::int64_t entry) {
            return (entry >= eofCluster);
        }

        std::int64_t getEofMarker() {
            return eofMarker;
        }

        float getEntrySize() {
            return entrySize;
        }

        std::int64_t getBitMask() {
            return bitMask;
        }

        std::int64_t hashCode() {
            return (std::int64_t) this;
        }
    };

    class Fat16Type : public FatType {

    public:
        Fat16Type() : FatType((1 << 16) - 16, 0xFFFFL, 2.0f, "FAT16   ") {}

        std::int64_t readEntry(std::vector<char> &data, std::int32_t index) override {
            std::int32_t idx = index << 1;
            std::int32_t b1 = data[idx] & 0xFF;
            std::int32_t b2 = data[idx + 1] & 0xFF;
            auto result = (b2 << 8) | b1;
            return result;
        }


        void writeEntry(std::vector<char> &data, std::int32_t index, std::int64_t entry) override {
            std::int32_t idx = index << 1;
            data[idx] = (char) (entry & 0xFF);
            data[idx + 1] = (char) ((entry >> 8) & 0xFF);
        }
    };

    class Fat32Type : FatType {

    public:
        Fat32Type() : FatType((1 << 28) - 16, 0xFFFFFFFFL, 4.0f, "FAT32   ") {}

        std::int64_t readEntry(std::vector<char> &data, std::int32_t index) override {
            std::int32_t idx = index * 4;
            std::int64_t l1 = data[idx] & 0xFF;
            std::int64_t l2 = data[idx + 1] & 0xFF;
            std::int64_t l3 = data[idx + 2] & 0xFF;
            std::int64_t l4 = data[idx + 3] & 0xFF;
            return (l4 << 24) | (l3 << 16) | (l2 << 8) | l1;
        }


        void writeEntry(std::vector<char> &data, std::int32_t index, std::int64_t entry) override {
            std::int32_t idx = index << 2;
            data[idx] = (char) (entry & 0xFF);
            data[idx + 1] = (char) ((entry >> 8) & 0xFF);
            data[idx + 2] = (char) ((entry >> 16) & 0xFF);
            data[idx + 3] = (char) ((entry >> 24) & 0xFF);
        }
    };
}
