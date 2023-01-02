#pragma once

#include "../util/ByteBuffer.hpp"
#include "../BlockDevice.hpp"

#include <memory>
#include <string>
#include <utility>

namespace akaifat::fat {
class Sector {
private:
    std::shared_ptr<BlockDevice> device;
    std::int64_t offset;
    
    bool dirty;
    
protected:
    ByteBuffer buffer;

    Sector(std::shared_ptr<BlockDevice> deviceToUse, std::int64_t _offset, std::int32_t size)
    : device (std::move(deviceToUse)), offset (_offset), buffer (ByteBuffer(size)), dirty (true)
    {
    }
    
    void markDirty() {
        dirty = true;
    }

    std::int32_t get16(std::int32_t atOffset) {
        return buffer.getShort(atOffset) & 0xffff;
    }

    std::int64_t get32(std::int32_t atOffset) {
        return buffer.getInt(atOffset);
    }
    
    unsigned char get8(std::int32_t atOffset) {
        return buffer.get(atOffset) & 0xff;
    }
    
    void set16(std::int32_t atOffset, std::int32_t value) {
        buffer.putShort(atOffset, (short) (value & 0xffff));
        dirty = true;
    }

    void set32(std::int32_t atOffset, std::int64_t value) {
        buffer.putInt(atOffset, (std::int32_t) (value & 0xffffffff));
        dirty = true;
    }

    void set8(std::int32_t atOffset, std::int32_t value) {
        if ((value & 0xff) != value) {
            throw std::runtime_error(std::to_string(value) + " too big to be stored in a single octet");
        }
        
        buffer.put(atOffset, (char) (value & 0xff));
        dirty = true;
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
