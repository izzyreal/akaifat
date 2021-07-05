#pragma once

#include "../AbstractFsObject.hpp"
#include "../FsDirectory.hpp"

#include "FatDirectoryEntry.hpp"
#include "ClusterChainDirectory.hpp"
#include "Fat.hpp"
#include "FatFile.hpp"

#include <memory>
#include <set>

namespace akaifat { class FsDirectoryEntry; }

namespace akaifat::fat {

    class AbstractDirectory;

    class AkaiFatLfnDirectoryEntry;

    class AkaiFatLfnDirectory : public akaifat::AbstractFsObject, public akaifat::FsDirectory {
    public:
        std::shared_ptr<AbstractDirectory> dir;
        std::map<std::string, std::shared_ptr<AkaiFatLfnDirectoryEntry>> akaiNameIndex;

        AkaiFatLfnDirectory(std::shared_ptr<AbstractDirectory> dir, std::shared_ptr<Fat> fat, bool readOnly);

        std::shared_ptr<Fat> getFat();

        std::shared_ptr<FatFile> getFile(const std::shared_ptr<FatDirectoryEntry>& entry);

        std::shared_ptr<AkaiFatLfnDirectory> getDirectory(const std::shared_ptr<FatDirectoryEntry>& entry);

        std::shared_ptr<akaifat::FsDirectoryEntry> addFile(std::string &name) override;

        bool isFreeName(std::string &name);

        static std::vector<std::string> splitName(std::string &s);

        std::shared_ptr<FsDirectoryEntry> addDirectory(std::string &name) override;

        std::shared_ptr<FsDirectoryEntry> getEntry(std::string &name) override;

        void flush() override;

        void remove(std::string &name) override;

        std::shared_ptr<AkaiFatLfnDirectoryEntry>
        unlinkEntry(std::string &entryName, bool isFile, std::shared_ptr<FatDirectoryEntry> realEntry);

        void linkEntry(const std::shared_ptr<AkaiFatLfnDirectoryEntry> &entry);

    private:
        std::set<std::string> usedAkaiNames;
        std::shared_ptr<Fat> fat;
        std::map<std::shared_ptr<FatDirectoryEntry>, std::shared_ptr<FatFile>> entryToFile;
        std::map<std::shared_ptr<FatDirectoryEntry>, std::shared_ptr<AkaiFatLfnDirectory>> entryToDirectory;

        void checkUniqueName(std::string &name);

        void parseLfn();

        void updateLFN();

        static std::shared_ptr<ClusterChainDirectory> read(const std::shared_ptr<FatDirectoryEntry>&, Fat *);

        bool isDirValid() override;

        bool isDirReadOnly() override;
    };
}
