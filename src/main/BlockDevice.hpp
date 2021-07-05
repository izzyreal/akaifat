#pragma once

#include "ByteBuffer.hpp"

namespace akaifat {
class BlockDevice {
public:
    virtual long getSize() = 0;

    virtual void read(long devOffset, ByteBuffer& dest) = 0;

    virtual void write(long devOffset, ByteBuffer& src) = 0;
            
    virtual void flush() = 0;

    virtual int getSectorSize() = 0;

    virtual void close() = 0;

    virtual bool isClosed() = 0;
    
    virtual bool isReadOnly() = 0;
    
};
}
