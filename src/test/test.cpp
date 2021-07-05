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

TEST_CASE("create disk image", "[image]") {
    std::remove("tmpakaifat.img");
    std::fstream img;
    img.open("tmpakaifat.img", std::ios_base::out);
    
    char bytes[1] {' '};
    for (int i = 0; i < 16 * 1024 * 1024; i++)
        img.write(bytes, 1);
    
    img.close();
    
    img.open("tmpakaifat.img", std::ios_base::out | std::ios_base::in);
    
    ImageBlockDevice device(img);

    SuperFloppyFormatter formatter(device);
    formatter.setVolumeLabel("foo");
    formatter.format();
    
    img.close();
    REQUIRE(TRUE);
}

/*
AkaiFatTestsFixture::AkaiFatTestsFixture()
{
    img.open("/Users/izmar/Desktop/fat16.img", std::ios_base::in | std::ios_base::out);

    img.seekg(std::ios::beg);

    device = std::make_shared<ImageBlockDevice>(img);
    fs = dynamic_cast<AkaiFatFileSystem *>(FileSystemFactory::createAkai(device.get(), false));

    root = std::dynamic_pointer_cast<AkaiFatLfnDirectory>(fs->getRoot());
}

AkaiFatTestsFixture::~AkaiFatTestsFixture()
{
    img.close();
}

int AkaiFatTestsFixture::uniqueID = 0;

TEST_CASE_METHOD(AkaiFatTestsFixture, "akaifat can read", "[read]") {
    auto bs = std::dynamic_pointer_cast<Fat16BootSector>(fs->getBootSector());

    auto volumeLabel = StrUtil::trim_copy(bs->getVolumeLabel());

    REQUIRE(volumeLabel == "MPC2000XL");
    
    auto entries = root->akaiNameIndex;

    printf("- %s ROOT LISTING -\n", volumeLabel.c_str());

    for (auto &e : entries)
        printf("Name: %s\n", e.first.c_str());

    auto test1 = entries["test1"];

    auto dir = std::dynamic_pointer_cast<AkaiFatLfnDirectory>(test1->getDirectory());
    printf("- TEST1 LISTING -\n");
    auto dirEntries = dir->akaiNameIndex;

    for (auto &e : dirEntries) {

        printf("Name: %s\n", e.second->getName().c_str());
        if (e.second->isFile())
        {
            auto length = e.second->getFile()->getLength();
            printf("Length: %li\n", length);
        }
        if (e.second->getName() == "SNARE4.SND") {
//            ByteBuffer newData(100);
//            for (int i = 0; i < newData.capacity(); i++)
//                newData.put((char)(i));
//            newData.flip();
//            e.second->getFile()->setLength(100);
//            e.second->getFile()->write(0, newData);
//            e.second->getFile()->flush();
            printf("SNARE4.SND data:");
            ByteBuffer buf(e.second->getFile()->getLength());
            e.second->getFile()->read(0, buf);
            buf.rewind();
            for (int i = 0; i < 100; i++)
                printf("%c", buf.get());
            printf("\n");

//            std::fstream output;
//            output.open("/Users/izmar/Desktop/SNARE5.SND", std::ios_base::out | std::ios_base::binary);
//            auto buf_ = buf.getBuffer();
//            output.write(&buf_[0], buf.capacity());
//            output.close();
        }
    }

//    std::string newDirName = "aaaa";
//    root->addDirectory(newDirName);


//    std::string toRemove = "snare4.snd";
//    dir->remove(toRemove);
//    std::string newFileName = "SNARE4.SND";
//    dir->addFile(newFileName);

    fs->flush();
}
*/
