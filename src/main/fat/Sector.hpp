#pragma once

#include "../util/ByteBuffer.hpp"
#include "../BlockDevice.hpp"

#include <memory>
#include <string>

namespace akaifat::fat {
class Sector {
private:
    std::shared_ptr<BlockDevice> device;
    std::int64_t offset;
    
    bool dirty;
    
protected:
    ByteBuffer buffer;

    Sector(std::shared_ptr<BlockDevice> _device, std::int64_t _offset, std::int32_t size)
    : device (_device), offset (_offset), buffer (ByteBuffer(size)), dirty (true)
    {
    }
    
    void markDirty() {
        dirty = true;
    }

    std::int32_t get16(std::int32_t offset) {
        return buffer.getShort(offset) & 0xffff;
    }

    std::int64_t get32(std::int32_t offset) {
        return buffer.getInt(offset);
    }
    
    unsigned char get8(std::int32_t offset) {
        return buffer.get(offset) & 0xff;
    }
    
    void set16(std::int32_t offset, std::int32_t value) {
        buffer.putShort(offset, (short) (value & 0xffff));
        dirty = true;
    }

    void set32(std::int32_t offset, std::int64_t value) {
//        buffer.putInt(offset, (std::uint32_t) (value & 0xffffffff));
        dirty = true;
    }

    void set8(std::int32_t offset, std::int32_t value) {
        if ((value & 0xff) != value) {
            throw std::runtime_error(std::to_string(value) + " too big to be stored in a single octet");
        }
        
        buffer.put(offset, (char) (value & 0xff));
        dirty = true;
    }
    
    std::int64_t getOffset() {
        return offset;
    }

public:
    void read_() {
        buffer.rewind();
        buffer.limit(buffer.capacity());
        device->read(offset, buffer);
        dirty = false;
    }
    
    bool isDirty() {
        return dirty;
    }
    
    std::shared_ptr<BlockDevice> getDevice() {
        return device;
    }

    void write() {
        if (!isDirty()) return;
        
        buffer.position(0);
        buffer.limit(buffer.capacity());
        device->write(offset, buffer);
        dirty = false;
    }
};
}
