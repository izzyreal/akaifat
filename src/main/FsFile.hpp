#pragma once

#include "ByteBuffer.hpp"

#include <exception>

namespace akaifat {
class FsFile {

public:
    virtual long getLength() = 0;

    virtual void setLength(long length) = 0;

    virtual void read(long offset, ByteBuffer& dest) = 0;

    virtual void write(long offset, ByteBuffer& src) = 0;
            
    virtual void flush() = 0;
};
}
