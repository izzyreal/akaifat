#pragma once

#include "../AbstractFsObject.hpp"

#include "AbstractDirectory.hpp"
#include "../util/ByteBuffer.hpp"
#include "LittleEndian.hpp"
#include "ShortName.hpp"

#include <utility>
#include <vector>
#include <cassert>

using namespace akaifat;

namespace akaifat::fat {
    class FatDirectoryEntry : public AbstractFsObject {

    private:
        bool dirty{};
        static const std::int32_t OFFSET_ATTRIBUTES = 0x0b;
        static const std::int32_t OFFSET_FILE_SIZE = 0x1c;
        static const std::int32_t F_READONLY = 0x01;
        static const std::int32_t F_HIDDEN = 0x02;
        static const std::int32_t F_SYSTEM = 0x04;
        static const std::int32_t F_VOLUME_ID = 0x08;
        static const std::int32_t F_DIRECTORY = 0x10;
        static const std::int32_t F_ARCHIVE = 0x20;

    public:
        explicit FatDirectoryEntry()
                : FatDirectoryEntry(std::vector<char>(SIZE), false) {
        }

        FatDirectoryEntry(std::vector<char> _data, bool readOnly)
                : AbstractFsObject(readOnly), data(std::move(_data)) {
        }

        void setFlag(std::int32_t mask, bool set) {
            std::int32_t oldFlags = getFlags();

            if (((oldFlags & mask) != 0) == set) return;

            if (set) {
                setFlags(oldFlags | mask);
            } else {
                setFlags(oldFlags & ~mask);
            }

            dirty = true;
        }

        std::int32_t getFlags() {
            return LittleEndian::getUInt8(data, OFFSET_ATTRIBUTES);
        }

        void setFlags(std::int32_t flags) {
            LittleEndian::setInt8(data, OFFSET_ATTRIBUTES, flags);
        }

        std::vector<char> data;

        static std::int32_t const SIZE = 32;
        static std::int32_t const ENTRY_DELETED_MAGIC = 0xe5;

        static std::shared_ptr<FatDirectoryEntry> read(ByteBuffer &buff, bool readOnly) {

            assert (buff.remaining() >= SIZE);

            if (buff.get(buff.position()) == 0)
                return {};

            std::vector<char> data(SIZE);
            buff.get(data);
            return std::make_shared<FatDirectoryEntry>(data, readOnly);
        }

        static void writeNullEntry(ByteBuffer &buff) {
            for (std::int32_t i = 0; i < SIZE; i++) {
                buff.put((char) 0);
            }
        }

        bool isVolumeLabel() {
            if (isLfnEntry()) return false;
            else return ((getFlags() & (F_DIRECTORY | F_VOLUME_ID)) == F_VOLUME_ID);
        }

        bool isSystemFlag() {
            return ((getFlags() & F_SYSTEM) != 0);
        }

        void setSystemFlag(bool isSystem) {
            setFlag(F_SYSTEM, isSystem);
        }

        bool isArchiveFlag() {
            return ((getFlags() & F_ARCHIVE) != 0);
        }

        void setArchiveFlag(bool isArchive) {
            setFlag(F_ARCHIVE, isArchive);
        }

        bool isHiddenFlag() {
            return ((getFlags() & F_HIDDEN) != 0);
        }

        void setHiddenFlag(bool isHidden) {
            setFlag(F_HIDDEN, isHidden);
        }

        bool isVolumeIdFlag() {
            return ((getFlags() & F_VOLUME_ID) != 0);
        }

        bool isReadonlyFlag() {
            return ((getFlags() & F_READONLY) != 0);
        }

        bool isLfnEntry() {
            return isReadonlyFlag() && isSystemFlag() &&
                   isHiddenFlag() && isVolumeIdFlag();
        }

        [[nodiscard]] bool isDirty() const {
            return dirty;
        }

        bool isDirectory() {
            return ((getFlags() & (F_DIRECTORY | F_VOLUME_ID)) == F_DIRECTORY);
        }

        static std::shared_ptr<FatDirectoryEntry> create(bool directory) {
            auto result = std::make_shared<FatDirectoryEntry>();

            if (directory) {
                result->setFlags(F_DIRECTORY);
            }

            return result;
        }

        static std::shared_ptr<FatDirectoryEntry> createVolumeLabel(const std::string &volumeLabel) {

            assert(volumeLabel.length() != 0);

            std::vector<char> data(SIZE);

            for (std::int32_t i = 0; i < volumeLabel.length(); i++)
                data[i] = volumeLabel[i];

            auto result = std::make_shared<FatDirectoryEntry>(data, false);
            result->setFlags(FatDirectoryEntry::F_VOLUME_ID);
            return result;
        }

        std::string getVolumeLabel() {
            if (!isVolumeLabel())
                throw std::runtime_error("not a volume label");

            std::string result;

            for (std::int32_t i = 0; i < AbstractDirectory::MAX_LABEL_LENGTH; i++) {
                auto b = data[i];

                if (b != 0) {
                    result.push_back((char) b);
                } else {
                    break;
                }
            }

            return result;
        }

        bool isDeleted() {
            return (LittleEndian::getUInt8(data, 0) == ENTRY_DELETED_MAGIC);
        }

        std::int64_t getLength() {
            return LittleEndian::getUInt32(data, OFFSET_FILE_SIZE);
        }

        void setLength(std::int64_t length) {
            LittleEndian::setInt32(data, OFFSET_FILE_SIZE, length);
        }

        ShortName getShortName() {
            if (data[0] == 0) {
                return {"", ""};
            } else {
                return ShortName::parse(data);
            }
        }

        bool isFile() {
            return ((getFlags() & (F_DIRECTORY | F_VOLUME_ID)) == 0);
        }

        void setShortName(ShortName &sn) {
            sn.write(data);
            dirty = true;
        }

        void setAkaiName(std::string s);

        std::int64_t getStartCluster() {
            return LittleEndian::getUInt16(data, 0x1a);
        }

        void setStartCluster(std::int64_t startCluster) {
            if (startCluster > INT_MAX)
                throw std::runtime_error("startCluster too big");

            LittleEndian::setInt16(data, 0x1a, (std::int32_t) startCluster);
        }

        void write(ByteBuffer &buff) {
            buff.put(data);
            dirty = false;
        }

        void setReadonlyFlag(bool isReadonly) {
            setFlag(F_READONLY, isReadonly);
        }
        
        std::string getLfnPart() {
                char unicodechar[13];

                unicodechar[0] = (char) LittleEndian::getUInt16(data, 1) & 0xff;
                unicodechar[1] = (char) LittleEndian::getUInt16(data, 3) & 0xff;
                unicodechar[2] = (char) LittleEndian::getUInt16(data, 5) & 0xff;
                unicodechar[3] = (char) LittleEndian::getUInt16(data, 7) & 0xff;
                unicodechar[4] = (char) LittleEndian::getUInt16(data, 9) & 0xff;
                unicodechar[5] = (char) LittleEndian::getUInt16(data, 14) & 0xff;
                unicodechar[6] = (char) LittleEndian::getUInt16(data, 16) & 0xff;
                unicodechar[7] = (char) LittleEndian::getUInt16(data, 18) & 0xff;
                unicodechar[8] = (char) LittleEndian::getUInt16(data, 20) & 0xff;
                unicodechar[9] = (char) LittleEndian::getUInt16(data, 22) & 0xff;
                unicodechar[10] = (char) LittleEndian::getUInt16(data, 24) & 0xff;
                unicodechar[11] = (char) LittleEndian::getUInt16(data, 28) & 0xff;
                unicodechar[12] = (char) LittleEndian::getUInt16(data, 30) & 0xff;

                std::int32_t end = 0;

                while ((end < 13) && (unicodechar[end] != '\0')) {
                    end++;
                }
                
                return std::string(unicodechar).substr(0, end);
            }
    };
}
