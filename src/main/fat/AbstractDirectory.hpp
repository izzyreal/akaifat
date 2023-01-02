#pragma once

#include "../util/ByteBuffer.hpp"

#include <memory>
#include <vector>
#include <string>

namespace akaifat::fat {

    class Fat;
    class FatDirectoryEntry;

    class AbstractDirectory {
    public:
        virtual ~AbstractDirectory() = default;

        static const std::int32_t MAX_LABEL_LENGTH = 11;

        void setEntries(std::vector<std::shared_ptr<FatDirectoryEntry>> &newEntries);

        std::shared_ptr<FatDirectoryEntry> getEntry(std::int32_t idx);

        [[nodiscard]] std::int32_t getCapacity() const;

        std::int32_t getEntryCount();

        [[nodiscard]] bool isRoot() const;

        std::int32_t getSize();

        void flush();

        void addEntry(std::shared_ptr<FatDirectoryEntry>);

        void removeEntry(const std::shared_ptr<FatDirectoryEntry>&);

        std::string &getLabel();

        std::shared_ptr<FatDirectoryEntry> createSub(Fat *fat);

        void setLabel(std::string &label);

        virtual void changeSize(std::int32_t entryCount) = 0;

    private:
        std::vector<std::shared_ptr<FatDirectoryEntry>> entries;
        bool readOnly;
        bool _isRoot;

        std::int32_t capacity;
        std::string volumeLabel;

        void checkRoot() const;

    public:
        AbstractDirectory(std::int32_t _capacity, bool _readOnly, bool _root);

        virtual void read(ByteBuffer &data) = 0;

        virtual void write(ByteBuffer &data) = 0;

        virtual std::int64_t getStorageCluster() = 0;

        virtual void sizeChanged(std::int64_t newSize);

        virtual void read();

    };
}
