#pragma once

#include "../ByteBuffer.hpp"

#include <vector>
#include <string>

namespace akaifat::fat {

    class Fat;
    class FatDirectoryEntry;

    class AbstractDirectory {
    public:
        virtual ~AbstractDirectory() = default;

        static const int MAX_LABEL_LENGTH = 11;

        void setEntries(std::vector<std::shared_ptr<FatDirectoryEntry>> &newEntries);

        std::shared_ptr<FatDirectoryEntry> getEntry(int idx);

        [[nodiscard]] int getCapacity() const;

        int getEntryCount();

        [[nodiscard]] bool isRoot() const;

        int getSize();

        void flush();

        void addEntry(std::shared_ptr<FatDirectoryEntry>);

        void removeEntry(const std::shared_ptr<FatDirectoryEntry>&);

        std::string &getLabel();

        std::shared_ptr<FatDirectoryEntry> createSub(Fat *fat);

        void setLabel(std::string &label);

        virtual void changeSize(int entryCount) = 0;

    private:
        std::vector<std::shared_ptr<FatDirectoryEntry>> entries;
        bool readOnly;
        bool _isRoot;

        int capacity;
        std::string volumeLabel;

        void checkRoot() const;

    public:
        AbstractDirectory(int _capacity, bool _readOnly, bool _root);

        virtual void read(ByteBuffer &data) = 0;

        virtual void write(ByteBuffer &data) = 0;

        virtual long getStorageCluster() = 0;

        virtual void sizeChanged(long newSize);

        virtual void read();

    };
}
