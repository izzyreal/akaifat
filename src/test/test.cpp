#include "test.hpp"

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_DISABLE_EXCEPTIONS

#include <catch2/catch.hpp>

#include "util/SuperFloppyFormatter.hpp"
#include "ImageBlockDevice.hpp"
#include "FileSystemFactory.hpp"

#include "fat/AkaiFatLfnDirectoryEntry.hpp"

#include "util/RemovableVolumes.h"
#include "util/VolumeMounter.h"

using namespace akaifat;
using namespace akaifat::fat;
using namespace akaifat::util;

const auto IMAGE_NAME = "tmpakaifat.img";
const auto IMAGE_SIZE = 16 * 1024 * 1024;

void createImage()
{
    std::remove(IMAGE_NAME);

    std::fstream img;
    img.open(IMAGE_NAME, std::ios_base::out);

    char bytes[1] {'\0'};
    for (int i = 0; i < IMAGE_SIZE; i++)
        img.write(bytes, 1);

    img.close();

    img.open(IMAGE_NAME, std::ios_base::out | std::ios_base::in);

    auto device = std::make_shared<ImageBlockDevice>(img, IMAGE_SIZE);

    SuperFloppyFormatter formatter(device);
    formatter.setVolumeLabel("MPC2000XL");
    formatter.format();

    img.close();
}

TEST_CASE("list removable volumes", "[volumes]")
{
    RemovableVolumes removableVolumes;

    class TestChangeListener : public VolumeChangeListener {
    public:
        std::vector<RemovableVolume> volumes;
        void processChange(RemovableVolume v) override {
            printf("We can do what we want now with: %s\n", v.deviceName.c_str());
            volumes.emplace_back(v);
        }
    };

    TestChangeListener listener;

    removableVolumes.addListener(&listener);

    removableVolumes.init();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    for (auto& v : listener.volumes)
    {
        auto name = v.deviceName;
        auto mediaSize = v.mediaSize;

        std::fstream volumeStream;

        volumeStream = VolumeMounter::mount(name, true);

        if (volumeStream.is_open()) {

            printf("Volume %s with UUID %s has been mounted\n", name.c_str(), v.volumeUUID.c_str());

            auto device = std::make_shared<ImageBlockDevice>(volumeStream, mediaSize);
            auto fs = dynamic_cast<AkaiFatFileSystem *>(FileSystemFactory::createAkai(device, true));
            auto root = std::dynamic_pointer_cast<AkaiFatLfnDirectory>(fs->getRoot());

            for (auto& e : root->akaiNameIndex) {
                printf("Entry: %s\n", e.first.c_str());
            }

            //std::string newDirName = "BBBB";
            //root->addDirectory(newDirName);
            //fs->close();

            volumeStream.close();
            VolumeMounter::unmount(name);

        }
        else {
            printf("Volume %s has NOT been mounted!\n", name.c_str());
            continue;
        }
    }
}

TEST_CASE("create disk image", "[image]") {
    bool success = true;

    try {
        createImage();
    } catch (const std::exception&) {
        success = false;
    }

    REQUIRE(success);
}

AkaiFatTestsFixture::AkaiFatTestsFixture()
{
    init();
}

void AkaiFatTestsFixture::init(bool create) {
    if (create) createImage();

    img.open(IMAGE_NAME, std::ios_base::in | std::ios_base::out);

    img.seekg(std::ios::beg);

    device = std::make_shared<ImageBlockDevice>(img, IMAGE_SIZE);
    fs = dynamic_cast<AkaiFatFileSystem *>(FileSystemFactory::createAkai(device, false));

    root = std::dynamic_pointer_cast<AkaiFatLfnDirectory>(fs->getRoot());
}

void AkaiFatTestsFixture::close() {
    fs->flush();
    fs->close();

    delete fs;

    img.flush();
    img.close();
}

AkaiFatTestsFixture::~AkaiFatTestsFixture() {
    close();
}

int AkaiFatTestsFixture::uniqueID = 0;

TEST_CASE_METHOD(AkaiFatTestsFixture, "akaifat can read", "[read]") {
    auto bs = std::dynamic_pointer_cast<Fat16BootSector>(fs->getBootSector());

    auto volumeLabel = StrUtil::trim_copy(bs->getVolumeLabel());

    REQUIRE(volumeLabel == "MPC2000XL");
}

TEST_CASE_METHOD(AkaiFatTestsFixture, "akaifat can write and read the written", "[write]") {
    // BEGIN Add directory
    std::string newDirName = "AAA";
    root->addDirectory(newDirName);
    close();
    // END Add directory

    // BEGIN Volume label
    init(false);

    auto bs = std::dynamic_pointer_cast<Fat16BootSector>(fs->getBootSector());
    auto volumeLabel = StrUtil::trim_copy(bs->getVolumeLabel());

    REQUIRE (volumeLabel == "MPC2000XL");
    // END Volume label

    // BEGIN Read new directory
    auto aaaEntry = root->getEntry(newDirName);
    REQUIRE (aaaEntry->isValid());
    REQUIRE (aaaEntry->getName() == newDirName);
    REQUIRE (aaaEntry->isDirectory());
    // END Read new directory

    // BEGIN Add file
    auto aaaDir = std::dynamic_pointer_cast<AkaiFatLfnDirectory>(aaaEntry->getDirectory());

    std::string newFileName = "TEST_WITH16CHARS.BIN";
    auto newFileEntry = aaaDir->addFile(newFileName);

    REQUIRE (newFileEntry);
    REQUIRE (newFileEntry->isValid());
    REQUIRE (newFileEntry->getName() == newFileName);
    REQUIRE (newFileEntry->isFile());

    auto newFile = newFileEntry->getFile();

    const auto FILE_LENGTH = 512;

    newFile->setLength(FILE_LENGTH);

    ByteBuffer src(FILE_LENGTH);

    for (int i = 0; i < FILE_LENGTH; i++)
        src.put(' ');
    src.flip();
    newFile->write(0, src);
    close();
    // END Add file

    // BEGIN Read and verify new file
    init(false);

    aaaEntry = root->getEntry(newDirName);
    aaaDir = std::dynamic_pointer_cast<AkaiFatLfnDirectory>(aaaEntry->getDirectory());

    newFileEntry = aaaDir->getEntry(newFileName);
    REQUIRE (newFileEntry);
    REQUIRE (newFileEntry->isValid());
    REQUIRE (newFileEntry->getName() == newFileName);
    REQUIRE (newFileEntry->isFile());

    newFile = aaaDir->getEntry(newFileName)->getFile();

    REQUIRE (newFile->getLength() == FILE_LENGTH);

    ByteBuffer dest(FILE_LENGTH);

    newFile->read(0, dest);

    bool isTheSame = true;

    src.rewind();
    dest.flip();

    for (int i = 0; i < FILE_LENGTH; i++) {
        if (src.get() != dest.get()) {
            isTheSame = false;
            break;
        }
    }

    REQUIRE (isTheSame);

    close();
    // END Read and verify new file

    // BEGIN Rename file
    init(false);

    aaaDir = std::dynamic_pointer_cast<AkaiFatLfnDirectory>(root->getEntry(newDirName)->getDirectory());
    newFileEntry = aaaDir->getEntry(newFileName);

    std::string renamedFileName = "FOOBAR.BIN";
    newFileEntry->setName(renamedFileName);

    close();
    init(false);

    aaaDir = std::dynamic_pointer_cast<AkaiFatLfnDirectory>(root->getEntry(newDirName)->getDirectory());

    REQUIRE (!aaaDir->getEntry(newFileName));
    REQUIRE (aaaDir->getEntry(renamedFileName));
    close();
    // END Rename file

    // BEGIN Move and rename file
    init(false);

    aaaDir = std::dynamic_pointer_cast<AkaiFatLfnDirectory>(root->getEntry(newDirName)->getDirectory());
    std::dynamic_pointer_cast<AkaiFatLfnDirectoryEntry>(aaaDir->getEntry(renamedFileName))->moveTo(root, newFileName);

    close();
    init(false);
    newFileEntry = root->getEntry(newFileName);
    REQUIRE(newFileEntry->isValid());
    REQUIRE(newFileEntry->getName() == newFileName);
    REQUIRE(newFileEntry->isFile());

    aaaDir = std::dynamic_pointer_cast<AkaiFatLfnDirectory>(root->getEntry(newDirName)->getDirectory());

    auto noNewFileEntry = aaaDir->getEntry(newFileName);
    REQUIRE(!noNewFileEntry);

    close();
    // END Move and rename file

    // BEGIN Remove file
    init(false);
    REQUIRE(root->getEntry(newFileName));
    root->remove(newFileName);

    close();
    init(false);
    REQUIRE (!root->getEntry(newFileName));
    // END Remove file
    // Don't call close() because it will be called by the fixture's destructor

}
