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

namespace akaifat::fat {
    class AkaiFatLfnDirectoryEntry : public akaifat::AbstractFsObject, public akaifat::FsDirectoryEntry {

    private:
        std::shared_ptr<AkaiFatLfnDirectory> parent;
        std::string fileName;

    public:
        AkaiFatLfnDirectoryEntry(const std::string &name, AkaiFatLfnDirectory *akaiFatLfnDirectory, bool directory)
                : AbstractFsObject(false), fileName(name), parent(akaiFatLfnDirectory) {
            realEntry = FatDirectoryEntry::create(directory);
            realEntry->setAkaiName(name);
        }

        bool isValid() override { return AbstractFsObject::isValid(); }

        bool isReadOnly() override { return AbstractFsObject::isReadOnly(); }

        AkaiFatLfnDirectoryEntry(AkaiFatLfnDirectory *akaiFatLfnDirectory, std::shared_ptr<FatDirectoryEntry> _realEntry,
                                 std::string _fileName)
                : AbstractFsObject(akaiFatLfnDirectory->isReadOnly()), parent(akaiFatLfnDirectory),
                  realEntry(std::move(_realEntry)), fileName(std::move(_fileName)) {
        }

        static std::shared_ptr<AkaiFatLfnDirectoryEntry> extract(AkaiFatLfnDirectory *dir, int offset, int len) {
            auto realEntry = dir->dir->getEntry(offset + len - 1);
            std::string shortName = realEntry->getShortName().asSimpleString();
            std::string akaiPart = StrUtil::trim_copy(AkaiPart::parse(realEntry->data).asSimpleString());
            std::string part1 = StrUtil::trim_copy(AkaiFatLfnDirectory::splitName(shortName)[0]);
            std::string ext = StrUtil::trim_copy(AkaiFatLfnDirectory::splitName(shortName)[1]);

            if (ext.length() > 0) ext = "." + ext;

            std::string akaiFileName = part1 + akaiPart + ext;

            return std::make_shared<AkaiFatLfnDirectoryEntry>(dir, realEntry, akaiFileName);
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
    };
}
