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

TEST_CASE("ByteBuffer reads signed little-endian shorts independently of char",
          "[bytebuffer]")
{
    for (const auto value : {-32768, -1, 0, 1, 32767})
    {
        CAPTURE(value);
        const auto bits = static_cast<uint16_t>(value);
        std::vector<char> bytes{0x55, static_cast<char>(bits & 255), static_cast<char>(bits >> 8), 0x55};
        ByteBuffer buffer(bytes);
        REQUIRE(buffer.getShort(1) == value);
        REQUIRE(buffer.position() == 2);
        buffer.position(1);
        REQUIRE(buffer.getShort() == value);
        REQUIRE(buffer.position() == 3);
    }
}

TEST_CASE("ByteBuffer reads full-width little-endian integers at odd offsets",
          "[bytebuffer]")
{
    for (const uint32_t value : {0u, 0x12345678u, 0x80000000u, 0xffffffffu})
    {
        CAPTURE(value);
        std::vector<char> bytes(5, 0x55);
        for (int b = 0; b < 4; ++b)
            bytes[b + 1] = static_cast<char>((value >> (b * 8)) & 255);
        ByteBuffer buffer(bytes);
        REQUIRE(buffer.getInt(1) == value);
        REQUIRE(buffer.position() == 4);
        buffer.position(1);
        REQUIRE(buffer.getInt() == value);
        REQUIRE(buffer.position() == 5);
    }
}
