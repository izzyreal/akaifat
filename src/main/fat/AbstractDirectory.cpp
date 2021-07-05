#include "AbstractDirectory.hpp"

#include "FatDirectoryEntry.hpp"
#include "ClusterChainDirectory.hpp"

using namespace akaifat::fat;

AbstractDirectory::AbstractDirectory(int _capacity, bool _readOnly, bool _root)
        : capacity(_capacity), readOnly(_readOnly), _isRoot(_root) {
}

void AbstractDirectory::setEntries(std::vector<std::shared_ptr<FatDirectoryEntry>> &newEntries) {
    if (newEntries.size() > capacity)
        throw std::runtime_error("too many entries");

    entries = newEntries;
}

void AbstractDirectory::sizeChanged(long newSize) {
    long newCount = newSize / FatDirectoryEntry::SIZE;

    if (newCount > INT_MAX)
        throw std::runtime_error("directory too large");

    capacity = (int) newCount;
}

void AbstractDirectory::read() {
    ByteBuffer data(capacity *FatDirectoryEntry::SIZE);

    read(data);
    data.flip();

    for (int i = 0; i < capacity; i++) {
        auto e = FatDirectoryEntry::read(data, readOnly);

        if (e == nullptr) continue;

        if (e->isVolumeLabel()) {
            if (!_isRoot)
                throw std::runtime_error("volume label in non-root directory");

            volumeLabel = e->getVolumeLabel();
        } else {
            entries.push_back(e);
        }
    }
}

std::shared_ptr<FatDirectoryEntry> AbstractDirectory::getEntry(int idx) {
    return entries[idx];
}

int AbstractDirectory::getCapacity() const {
    return capacity;
}

int AbstractDirectory::getEntryCount() {
    return (int) entries.size();
}

bool AbstractDirectory::isRoot() const {
    return _isRoot;
}

int AbstractDirectory::getSize() {
    return (int) entries.size() + ((volumeLabel.length() != 0) ? 1 : 0);
}

void AbstractDirectory::flush() {
    ByteBuffer data(capacity *FatDirectoryEntry::SIZE
    +(volumeLabel.length() != 0 ? FatDirectoryEntry::SIZE : 0));

    for (const auto& entry : entries) {
        if (entry != nullptr)
            entry->write(data);
    }

    if (volumeLabel.length() != 0) {
        auto labelEntry =
                FatDirectoryEntry::createVolumeLabel(volumeLabel);

        labelEntry->write(data);
    }

    if (data.hasRemaining())
        FatDirectoryEntry::writeNullEntry(data);

    data.flip();

    write(data);
}

void AbstractDirectory::addEntry(std::shared_ptr<FatDirectoryEntry> e) {
    assert (e != nullptr);

    if (getSize() == capacity)
        changeSize(capacity + 1);

    entries.push_back(e);
}

void AbstractDirectory::removeEntry(const std::shared_ptr<FatDirectoryEntry>& entry) {
    assert (entry != nullptr);

    auto it = find(begin(entries), end(entries), entry);

    if (it != end(entries))
        entries.erase(it);

    changeSize(getSize());
}

std::string &AbstractDirectory::getLabel() {
    checkRoot();

    return volumeLabel;
}

std::shared_ptr<FatDirectoryEntry> AbstractDirectory::createSub(Fat *fat) {
    auto chain = std::make_shared<ClusterChain>(fat, false);
    chain->setChainLength(1);

    auto entry = FatDirectoryEntry::create(true);
    entry->setStartCluster(chain->getStartCluster());

    ClusterChainDirectory dir(chain, false);

    auto dot = FatDirectoryEntry::create(true);
    auto sn_dot = ShortName::DOT();
    dot->setShortName(sn_dot);
    dot->setStartCluster(dir.getStorageCluster());
    dir.addEntry(dot);

    auto dotDot = FatDirectoryEntry::create(true);
    dotDot->setShortName(ShortName::DOT_DOT());
    dotDot->setStartCluster(getStorageCluster());
    dir.addEntry(dotDot);

    dir.flush();

    return entry;
}

void AbstractDirectory::setLabel(std::string &label) {
    checkRoot();

    if (label.length() > MAX_LABEL_LENGTH)
        throw std::runtime_error("label too long");

    volumeLabel = label;
}

void AbstractDirectory::checkRoot() const {
    if (!isRoot())
        throw std::runtime_error("only supported on root directories");
}
