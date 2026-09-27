#include "BootSector.hpp"

#include "Fat16BootSector.hpp"

using namespace akaifat;
using namespace akaifat::fat;

std::shared_ptr<BootSector> BootSector::read(const std::shared_ptr<BlockDevice>& device) {
    ByteBuffer bb(512);
    device->read(0, bb);
    
    const auto u16 = [&](int offset) { return bb.getShort(offset) & 0xffff; };
    // MPC60 floppies have an old BPB without the PC boot signature. Admit
    // this observed profile only, rather than accepting arbitrary unsigned BPBs.
    const bool legacyAkai = bb.get(0) == static_cast<char>(0xeb) &&
        bb.get(1) == 0x34 && bb.get(2) == static_cast<char>(0x90) &&
        u16(11) == 512 && bb.get(13) == 2 && u16(14) == 1 &&
        bb.get(16) == 2 && u16(17) == 112 && u16(19) == 1600 &&
        (bb.get(21) & 0xff) == 0xf9 && u16(22) == 3 &&
        u16(24) == 10 && u16(26) == 2 && u16(510) == 0 &&
        device->getSize() == 819200;
    if (u16(510) != 0xaa55 && !legacyAkai)
        throw std::runtime_error("missing boot sector signature");

    unsigned char sectorsPerCluster = bb.get(SECTORS_PER_CLUSTER_OFFSET);
    
    if (sectorsPerCluster <= 0)
        throw std::runtime_error("suspicious sectors per cluster count " + std::to_string(sectorsPerCluster));
                
    auto result = std::make_shared<Fat16BootSector>(device);
    result->read_();
    return result;
}
