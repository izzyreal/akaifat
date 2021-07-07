#include "test.hpp"

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_DISABLE_EXCEPTIONS

#include <catch2/catch.hpp>

#include "SuperFloppyFormatter.hpp"
#include "ImageBlockDevice.hpp"
#include "FileSystemFactory.hpp"

#include "fat/AkaiFatLfnDirectoryEntry.hpp"

using namespace akaifat;
using namespace akaifat::fat;

const auto IMAGE_NAME = "tmpakaifat.img";

void createImage()
{
    std::remove(IMAGE_NAME);
    
    std::fstream img;
    img.open(IMAGE_NAME, std::ios_base::out);
    
    char bytes[1] {'\0'};
    for (int i = 0; i < 16 * 1024 * 1024; i++)
        img.write(bytes, 1);
    
    img.close();
    
    img.open(IMAGE_NAME, std::ios_base::out | std::ios_base::in);
    
    ImageBlockDevice device(img);
    
    SuperFloppyFormatter formatter(device);
    formatter.setVolumeLabel("MPC2000XL");
    formatter.format();
    
    img.close();
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
    
    device = std::make_shared<ImageBlockDevice>(img);
    fs = dynamic_cast<AkaiFatFileSystem *>(FileSystemFactory::createAkai(device.get(), false));
    
    root = std::dynamic_pointer_cast<AkaiFatLfnDirectory>(fs->getRoot());
}

void AkaiFatTestsFixture::close() {
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
    std::string newDirName = "AAA";
    root->addDirectory(newDirName);
    fs->flush();
    close();
    init(false);
    
    auto bs = std::dynamic_pointer_cast<Fat16BootSector>(fs->getBootSector());
    auto volumeLabel = StrUtil::trim_copy(bs->getVolumeLabel());
    
    REQUIRE (volumeLabel == "MPC2000XL");
    
    auto aaaEntry = root->getEntry(newDirName);
    REQUIRE (aaaEntry->isValid());
    REQUIRE (aaaEntry->getName() == newDirName);
    REQUIRE (aaaEntry->isDirectory());
    
    auto aaaDir = std::dynamic_pointer_cast<AkaiFatLfnDirectory>(aaaEntry->getDirectory());
    
    std::string newFileName = "TEST_WITH16CHARS.BIN";
    auto newFileEntry = aaaDir->addFile(newFileName);
    
    auto newFile = newFileEntry->getFile();
    newFile->setLength(10000);
    
    ByteBuffer src(10000);
    
    for (int i = 0; i < 10000; i++)
        src.put(i % 256);
    src.flip();
    newFile->write(0, src);
    
    fs->flush();
    
    close();
    
    init(false);
    
    aaaEntry = root->getEntry(newDirName);
    aaaDir = std::dynamic_pointer_cast<AkaiFatLfnDirectory>(aaaEntry->getDirectory());
    
    newFileEntry = aaaDir->getEntry(newFileName);
    
    REQUIRE (newFileEntry->isValid());
    REQUIRE (newFileEntry->getName() == newFileName);
    REQUIRE (newFileEntry->isFile());
    
    newFile = aaaDir->getEntry(newFileName)->getFile();
    
    REQUIRE (newFile->getLength() == 10000);
    
    ByteBuffer dest(10000);
    
    newFile->read(0, dest);
    
    bool isTheSame = true;
    
    src.rewind();
    dest.flip();
    
    for (int i = 0; i < 10000; i++) {
        if (src.get() != dest.get()) {
            isTheSame = false;
            break;
        }
    }
    
    REQUIRE (isTheSame);
    
    close();
    
//    init(false);
    
//    aaaEntry = root->getEntry(newDirName);
//    aaaDir = std::dynamic_pointer_cast<AkaiFatLfnDirectory>(aaaEntry->getDirectory());
//    aaaDir->remove(newFileName);
//    fs->flush();
    
    //    aaaDir = std::dynamic_pointer_cast<AkaiFatLfnDirectory>(aaaEntry->getDirectory());
    
    //    REQUIRE (aaaDir->akaiNameIndex.find(newFileName) == end(aaaDir->akaiNameIndex));
    
    close();
}
