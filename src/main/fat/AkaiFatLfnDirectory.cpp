#include "AkaiFatLfnDirectory.hpp"

#include <utility>

#include "AkaiFatLfnDirectoryEntry.hpp"

using namespace akaifat::fat;
using namespace akaifat;

AkaiFatLfnDirectory::AkaiFatLfnDirectory(std::shared_ptr<AbstractDirectory> _dir, std::shared_ptr<Fat> _fat, bool readOnly)
        : AbstractFsObject(readOnly), dir(std::move(_dir)), fat(std::move(_fat)) {
    parseLfn();
}

bool AkaiFatLfnDirectory::isDirReadOnly() {
    return AbstractFsObject::isReadOnly();
}

bool AkaiFatLfnDirectory::isDirValid() {
    return AbstractFsObject::isValid();
}

std::shared_ptr<Fat> AkaiFatLfnDirectory::getFat() {
    return fat;
}

std::shared_ptr<FatFile> AkaiFatLfnDirectory::getFile(const std::shared_ptr<FatDirectoryEntry>& entry) {
    std::shared_ptr<FatFile> file;

    if (entryToFile.find(entry) == end(entryToFile)) {
        file = FatFile::get(fat.get(), entry);
        entryToFile[entry] = file;
    } else {
        file = entryToFile[entry];
    }

    return file;
}

std::shared_ptr<AkaiFatLfnDirectory> AkaiFatLfnDirectory::getDirectory(const std::shared_ptr<FatDirectoryEntry>& entry)
{
    std::shared_ptr<AkaiFatLfnDirectory> result;

    if (entryToDirectory.find(entry) == end(entryToDirectory)) {
        auto storage = read(entry, fat.get());
        result = std::make_shared<AkaiFatLfnDirectory>(storage, fat, isReadOnly());
        entryToDirectory[entry] = result;
    } else {
        result = entryToDirectory[entry];
    }

    return result;
}

std::shared_ptr<FsDirectoryEntry> AkaiFatLfnDirectory::addFile(std::string &name) {
    checkWritable();
    checkUniqueName(name);

    StrUtil::trim(name);

    auto entry = std::make_shared<AkaiFatLfnDirectoryEntry>(name, this, false);

    dir->addEntry(entry->realEntry);
    auto nameLower = StrUtil::to_lower_copy(name);
    akaiNameIndex[nameLower] = entry;

    getFile(entry->realEntry);

    return entry;
}

bool AkaiFatLfnDirectory::isFreeName(std::string &name) {
    return usedAkaiNames.find(StrUtil::to_lower_copy(name)) == usedAkaiNames.end();
}


std::vector<std::string> AkaiFatLfnDirectory::splitName(std::string &s) {
    if (s == ".") return {".", ""};
    if (s == "..") return {"..", ""};

    auto it = s.find_last_of('.');

    if (it == std::string::npos) return {s, ""};

    return {s.substr(0, it), s.substr(it + 1)};
}

std::shared_ptr<FsDirectoryEntry> AkaiFatLfnDirectory::addDirectory(std::string &_name) {
    checkWritable();
    checkUniqueName(_name);
    auto name = StrUtil::trim(_name);
    auto real = dir->createSub(fat.get());
    ShortName sn(name);
    real->setAkaiName(name);
    auto e = std::make_shared<AkaiFatLfnDirectoryEntry>(this, real, name);

    try {
        dir->addEntry(real);
    } catch (std::exception &ex) {
        ClusterChain cc(fat.get(), real->getStartCluster(), false);
        cc.setChainLength(0);
        dir->removeEntry(real);
        throw ex;
    }

    akaiNameIndex[StrUtil::to_lower_copy(name)] = e;

    getDirectory(real);

    flush();
    return e;
}

std::shared_ptr<FsDirectoryEntry> AkaiFatLfnDirectory::getEntry(std::string &name) {
    if (akaiNameIndex.find(name) == akaiNameIndex.end()) return {};
    return akaiNameIndex[StrUtil::to_lower_copy(name)];
}

void AkaiFatLfnDirectory::flush() {
    checkWritable();

    for (auto f : entryToFile)
        f.second->flush();

    for (auto d : entryToDirectory)
        d.second->flush();

    updateLFN();
    dir->flush();
}

void AkaiFatLfnDirectory::remove(std::string &name) {
    checkWritable();

    auto entry = getEntry(name);

    if (!entry) return;

    auto akaiEntry = std::dynamic_pointer_cast<AkaiFatLfnDirectoryEntry>(entry);
    auto entryName = akaiEntry->getName();
    auto isFile = akaiEntry->isFile();
    unlinkEntry(entryName, isFile, akaiEntry->realEntry);

    // Temporary helper object to modify the fat
    ClusterChain cc(fat.get(), akaiEntry->realEntry->getStartCluster(), false);
    cc.setChainLength(0);

    updateLFN();
}

std::shared_ptr<AkaiFatLfnDirectoryEntry>
AkaiFatLfnDirectory::unlinkEntry(std::string &entryName, bool isFile, std::shared_ptr<FatDirectoryEntry> realEntry) {
    if (entryName.length() == 0 || entryName[0] == '.') return {};

    std::string lowerName = StrUtil::to_lower_copy(entryName);

    assert(akaiNameIndex[lowerName]);

    auto unlinkedEntryRef = akaiNameIndex[lowerName];

    akaiNameIndex.erase(lowerName);

    assert(usedAkaiNames.find(lowerName) != usedAkaiNames.end());
    usedAkaiNames.erase(lowerName);

    if (isFile) {
        entryToFile.erase(realEntry);
    } else {
        entryToDirectory.erase(realEntry);
    }

    return unlinkedEntryRef;
}

void AkaiFatLfnDirectory::linkEntry(const std::shared_ptr<AkaiFatLfnDirectoryEntry> &entry) {
    auto name = entry->getName();
    checkUniqueName(name);
    entry->realEntry->setAkaiName(name);
    akaiNameIndex[StrUtil::to_lower_copy(name)] = entry;
    updateLFN();
}

void AkaiFatLfnDirectory::checkUniqueName(std::string &name) {
    std::string lowerName = StrUtil::to_lower_copy(name);

    if (!usedAkaiNames.emplace(lowerName).second) {
        throw std::runtime_error("an entry named " + name + " already exists");
    } else {
        usedAkaiNames.erase(lowerName);
    }
}

void AkaiFatLfnDirectory::parseLfn() {
    int i = 0;
    int size = dir->getEntryCount();

    while (i < size) {
        while (i < size &&
               (dir->getEntry(i) == nullptr || dir->getEntry(i)->getShortName().asSimpleString().length() == 0)) {
            i++;
        }

        if (i >= size) {
            break;
        }

        int offset = i;

        while (dir->getEntry(i)->isLfnEntry()) {
            i++;
            if (i >= size)
                break;
        }

        if (i >= size)
            break;

        auto current = AkaiFatLfnDirectoryEntry::extract(this, offset, ++i - offset);

        if (!current->realEntry->isDeleted() && current->isValid()) {
            auto name = current->getName();
            checkUniqueName(name);
            auto nameLower = StrUtil::to_lower_copy(name);
            usedAkaiNames.emplace(nameLower);
            akaiNameIndex[nameLower] = current;
        }
    }
}

void AkaiFatLfnDirectory::updateLFN() {
    std::vector<std::shared_ptr<FatDirectoryEntry>> dest;

    for (auto &currentEntry : akaiNameIndex) {
        dest.push_back(currentEntry.second->realEntry);
    }

    dir->changeSize(static_cast<int>(dest.size()));
    dir->setEntries(dest);
}

std::shared_ptr<ClusterChainDirectory> AkaiFatLfnDirectory::read(const std::shared_ptr<FatDirectoryEntry>& entry, Fat *fat) {
    if (!entry->isDirectory()) throw std::runtime_error(entry->getShortName().asSimpleString() + " is no directory");

    auto chain = std::make_shared<ClusterChain>(fat, entry->getStartCluster(), entry->isReadonlyFlag());

    auto result = std::make_shared<ClusterChainDirectory>(chain, false);

    result->read();

    return result;
}
