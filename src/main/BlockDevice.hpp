#pragma once

#include "util/ByteBuffer.hpp"

namespace akaifat {
class BlockDevice {
public:
    virtual std::int64_t getSize() = 0;

    virtual void read(std::int64_t devOffset, ByteBuffer& dest) = 0;

    virtual void write(std::int64_t devOffset, ByteBuffer& src) = 0;
            
    virtual void flush() = 0;

    virtual std::int32_t getSectorSize() = 0;

    virtual void close() = 0;

    virtual bool isClosed() = 0;
    
    virtual bool isReadOnly() = 0;
    
};
}
