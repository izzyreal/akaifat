#include "catch2/catch_test_macros.hpp"

#include "test.hpp"
#include "fat/Fat.hpp"
#include "FileSystemFactory.hpp"

#include "fat/AkaiFatLfnDirectoryEntry.hpp"

using namespace akaifat;
using namespace akaifat::fat;

TEST_CASE_METHOD(AkaiFatTestsFixture, "Fat::hashCode", "[fat]")
{
    auto bs = std::dynamic_pointer_cast<Fat16BootSector>(fs->getBootSector());
    auto emptyFatHashCode1 = root->getFat()->hashCode();
    auto emptyFatTypeHashCode1 = bs->getFatType()->hashCode();

    bs = std::dynamic_pointer_cast<Fat16BootSector>(fs->getBootSector());
    const auto emptyFatHashCode2 = root->getFat()->hashCode();
    const auto emptyFatTypeHashCode2 = bs->getFatType()->hashCode();

    REQUIRE(emptyFatHashCode1 == emptyFatHashCode2);
    REQUIRE(emptyFatTypeHashCode1 == emptyFatTypeHashCode2);

    std::string newDirName = "FOO";
    root->addDirectory(newDirName);

    const auto onedirFatHashCode1 = root->getFat()->hashCode();
    const auto onedirFatTypeHashCode1 = bs->getFatType()->hashCode();

    root->flush();
    close();
    init(false);

    const auto onedirFatHashCode2 = root->getFat()->hashCode();
    const auto onedirFatTypeHashCode2 = bs->getFatType()->hashCode();

    REQUIRE(onedirFatHashCode1 == onedirFatHashCode2);
    REQUIRE(onedirFatTypeHashCode1 == onedirFatTypeHashCode2);

    REQUIRE(onedirFatHashCode1 != emptyFatHashCode1);

    root->remove(newDirName);
    root->flush();

    const auto onedirRemovedFatHashCode = root->getFat()->hashCode();
    REQUIRE(onedirRemovedFatHashCode == emptyFatHashCode1);
}