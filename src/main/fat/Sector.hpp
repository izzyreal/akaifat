#pragma once

#include "../util/ByteBuffer.hpp"
#include "../BlockDevice.hpp"

#include <memory>
#include <string>

namespace akaifat::fat {
class Sector {
private:
    std::shared_ptr<BlockDevice> device;
    long offset;
    
    bool dirty;
    
protected:
    ByteBuffer buffer;

    Sector(std::shared_ptr<BlockDevice> _device, long _offset, int size)
    : device (_device), offset (_offset), buffer (ByteBuffer(size)), dirty (true)
    {
    }
    
    void markDirty() {
        dirty = true;
    }

    int get16(int offset) {
        return buffer.getShort(offset) & 0xffff;
    }

    long get32(int offset) {
        return buffer.getInt(offset);
    }
    
    unsigned char get8(int offset) {
        return buffer.get(offset) & 0xff;
    }
    
    void set16(int offset, int value) {
        buffer.putShort(offset, (short) (value & 0xffff));
        dirty = true;
    }

    void set32(int offset, long value) {
//        buffer.putInt(offset, (int) (value & 0xffffffff));
        dirty = true;
    }

    void set8(int offset, int value) {
        if ((value & 0xff) != value) {
            throw std::runtime_error(std::to_string(value) + " too big to be stored in a single octet");
        }
        
        buffer.put(offset, (char) (value & 0xff));
        dirty = true;
    }
    
    long getOffset() {
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
