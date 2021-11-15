#pragma once

#include "../AbstractFsObject.hpp"
#include "../FsDirectoryEntry.hpp"
#include "../FsDirectory.hpp"

#include "AkaiFatLfnDirectory.hpp"
#include "AkaiFatLfnDirectoryEntry.hpp"

#include <utility>

#include "FatDirectoryEntry.hpp"
#include "FatFile.hpp"
#include "AkaiPart.hpp"
#include "LittleEndian.hpp"

namespace akaifat::fat {
    class AkaiFatLfnDirectoryEntry : public akaifat::AbstractFsObject, public akaifat::FsDirectoryEntry {

    private:
        std::shared_ptr<AkaiFatLfnDirectory> parent;
        std::string fileName;
        
        size_t totalEntrySize() {
            size_t result = (fileName.length() / 13) + 1;
            if ((fileName.length() % 13) != 0) result++;
            return result;
        }

        static std::shared_ptr<FatDirectoryEntry> createPart(std::string subName,
                int ordinal, char checkSum, bool isLast) {
                
            char unicodechar[13];

            const char* unicodechar1 = subName.c_str();
            for (int i = 0; i < 13; i++) unicodechar[i] = unicodechar1[i];
            
            for (auto i=subName.length(); i < 13; i++) {
                if (i==subName.length()) {
                    unicodechar[i] = 0x0000;
                } else {
                    unicodechar[i] = static_cast<char>(0xffff);
                }
            }

            std::vector<char> rawData(FatDirectoryEntry::SIZE);
            
            if (isLast) {
                LittleEndian::setInt8(rawData, 0, ordinal + (1 << 6));
            } else {
                LittleEndian::setInt8(rawData, 0, ordinal);
            }
            
            LittleEndian::setInt16(rawData, 1, static_cast<unsigned char>(unicodechar[0]));
            LittleEndian::setInt16(rawData, 3, static_cast<unsigned char>(unicodechar[1]));
            LittleEndian::setInt16(rawData, 5, static_cast<unsigned char>(unicodechar[2]));
            LittleEndian::setInt16(rawData, 7, static_cast<unsigned char>(unicodechar[3]));
            LittleEndian::setInt16(rawData, 9, static_cast<unsigned char>(unicodechar[4]));
            LittleEndian::setInt8(rawData, 11, static_cast<unsigned char>(0x0f)); // this is the hidden
                                                      // attribute tag for lfn
            LittleEndian::setInt8(rawData, 12, 0); // reserved
            LittleEndian::setInt8(rawData, 13, static_cast<unsigned char>(checkSum & 0xff)); // checksum
            LittleEndian::setInt16(rawData, 14, static_cast<unsigned char>(unicodechar[5]));
            LittleEndian::setInt16(rawData, 16, static_cast<unsigned char>(unicodechar[6]));
            LittleEndian::setInt16(rawData, 18, static_cast<unsigned char>(unicodechar[7]));
            LittleEndian::setInt16(rawData, 20, static_cast<unsigned char>(unicodechar[8]));
            LittleEndian::setInt16(rawData, 22, static_cast<unsigned char>(unicodechar[9]));
            LittleEndian::setInt16(rawData, 24, static_cast<unsigned char>(unicodechar[10]));
            LittleEndian::setInt16(rawData, 26, 0); // sector... unused
            LittleEndian::setInt16(rawData, 28, static_cast<unsigned char>(unicodechar[11]));
            LittleEndian::setInt16(rawData, 30, static_cast<unsigned char>(unicodechar[12]));
            
            return std::make_shared<FatDirectoryEntry>(rawData, false);
        }
        
    public:
        AkaiFatLfnDirectoryEntry(const std::string &name, std::shared_ptr<AkaiFatLfnDirectory> akaiFatLfnDirectory, bool directory)
                : AbstractFsObject(false), parent(akaiFatLfnDirectory), fileName(name) {
            realEntry = FatDirectoryEntry::create(directory);
            realEntry->setAkaiName(name);
        }

        bool isValid() override { return AbstractFsObject::isValid(); }

        bool isReadOnly() override { return AbstractFsObject::isReadOnly(); }

        AkaiFatLfnDirectoryEntry(std::shared_ptr<AkaiFatLfnDirectory> akaiFatLfnDirectory, std::shared_ptr<FatDirectoryEntry> _realEntry,
                                 std::string _fileName)
                : AbstractFsObject(akaiFatLfnDirectory->isReadOnly()), parent(akaiFatLfnDirectory),
                  fileName(std::move(_fileName)), realEntry(std::move(_realEntry)) {
        }

        static std::shared_ptr<AkaiFatLfnDirectoryEntry> extract(std::shared_ptr<AkaiFatLfnDirectory> dir, int offset, int len) {
            auto realEntry = dir->dir->getEntry(offset + len - 1);
            std::string fileName;
            
            if (len == 1) {
                // Every single-length entry is treated like an Akai 16.3 FAT16 entry
                std::string shortName = realEntry->getShortName().asSimpleString();
                std::string akaiPart = AkaiStrUtil::trim_copy(AkaiPart::parse(realEntry->data).asSimpleString());
                std::string part1 = AkaiStrUtil::trim_copy(AkaiFatLfnDirectory::splitName(shortName)[0]);
                std::string ext = AkaiStrUtil::trim_copy(AkaiFatLfnDirectory::splitName(shortName)[1]);

                if (ext.length() > 0) ext = "." + ext;

                std::string akaiFileName = part1 + akaiPart + ext;
                fileName = akaiFileName;
            } else {
                /* stored in reverse order */
                std::string name;
                
                for (int i = len - 2; i >= 0; i--) {
                    auto entry = dir->dir->getEntry(i + offset);
                    name.append(entry->getLfnPart());
                }
                
                fileName = AkaiStrUtil::trim(name);
            }

            return std::make_shared<AkaiFatLfnDirectoryEntry>(dir, realEntry, fileName);
        }

        bool isHiddenFlag() {
            return realEntry->isHiddenFlag();
        }

        void setHiddenFlag(bool hidden) {
            checkWritable();
            realEntry->setHiddenFlag(hidden);
        }

        bool isSystemFlag() {
            return realEntry->isSystemFlag();
        }

        void setSystemFlag(bool systemEntry) {
            checkWritable();
            realEntry->setSystemFlag(systemEntry);
        }

        bool isReadOnlyFlag() {
            return realEntry->isReadonlyFlag();
        }

        void setReadOnlyFlag(bool readOnly) {
            checkWritable();
            realEntry->setReadonlyFlag(readOnly);
        }

        bool isArchiveFlag() {
            return realEntry->isArchiveFlag();
        }

        void setArchiveFlag(bool archive) {
            checkWritable();
            realEntry->setArchiveFlag(archive);
        }

        std::string getName() override {
            checkValid();

            return fileName;
        }
        
        std::string getAkaiName() {
            auto shortName = ShortName::parse(realEntry->data).asSimpleString();
            auto akaiPart = getAkaiPart();
            
            auto lastDot = shortName.find_last_of('.');
            bool hasDot = lastDot != std::string::npos;
            
            std::string name = hasDot ? shortName.substr(0, lastDot) : shortName;
            std::string ext = hasDot ? shortName.substr(lastDot + 1) : "";
            
            std::string finalName = name + akaiPart;
            
            if (hasDot) finalName += ("." + ext);
            
            return finalName;
        }

        std::string getAkaiPart() {
            if (isDirectory()) return "";
            return AkaiPart::parse(realEntry->data).asSimpleString();
        }

        void setAkaiPart(std::string s) {
            if (isDirectory()) return;
            AkaiPart ap(s);
            ap.write(realEntry->data);
        }

        std::shared_ptr<akaifat::FsDirectory> getParent() override {
            checkValid();
            return parent;
        }

        void setName(std::string newName) override {
            checkWritable();

            if (!parent->isFreeName(newName)) {
                throw std::runtime_error("the name \"" + newName + "\" is already in use");
            }

            auto entryName = getName();

            auto unlinkedEntryRef = parent->unlinkEntry(entryName, isFile(), realEntry);
            fileName = newName;
            parent->linkEntry(unlinkedEntryRef);
        }

        void moveTo(std::shared_ptr<AkaiFatLfnDirectory> target, std::string newName) {

            checkWritable();

            if (!target->isFreeName(newName)) {
                throw std::runtime_error("the name \"" + newName + "\" is already in use");
            }

            auto entryName = getName();

            auto unlinkedEntryRef = parent->unlinkEntry(entryName, isFile(), realEntry);
            parent = target;
            fileName = newName;
            parent->linkEntry(unlinkedEntryRef);
        }

        std::shared_ptr<FsFile> getFile() override {
            return parent->getFile(realEntry);
        }
        
        std::shared_ptr<akaifat::FsDirectory> getDirectory() override {
            return parent->getDirectory(realEntry);
        }

        bool isFile() override {
            return realEntry->isFile();
        }

        bool isDirectory() override {
            return realEntry->isDirectory();
        }

        bool isDirty() override {
            return realEntry->isDirty();
        }

        std::shared_ptr<FatDirectoryEntry> realEntry;
        
        std::vector<std::shared_ptr<FatDirectoryEntry>> compactForm() {
            std::vector<std::shared_ptr<FatDirectoryEntry>> result;
            
            auto sn = realEntry->getShortName();
            
            if (sn.equals(ShortName::DOT()) || sn.equals(ShortName::DOT_DOT()))
            {
                result.push_back(realEntry);
                return result;
            }
            
            if (ShortName::canConvert(fileName) && ShortName::get(fileName).asSimpleString() == fileName)
            {
                auto sn2 = ShortName::get(fileName);
                realEntry->setShortName(sn2);
                result.push_back(realEntry);
                return result;
            }
            
            const size_t entrySize = static_cast<size_t>(totalEntrySize());
            
            result.resize(entrySize);
            
            const char checkSum = realEntry->getShortName().checkSum();
            
            int j = 0;
            
            for (size_t i = entrySize - 2; i > 0; i--)
            {
                result[i] = createPart(fileName.substr(j * 13, j * 13 + 13), j  + 1, checkSum, false);
                j++;
            }
            
            result[0] = createPart(fileName.substr(j * 13),
                    j + 1, checkSum, true);
            
            result[entrySize - 1] = realEntry;
            
            return result;
        }
    };
}
