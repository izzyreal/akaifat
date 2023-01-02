#pragma once

#include "util/ByteBuffer.hpp"

#include <exception>

namespace akaifat {
class FsFile {

public:
    virtual std::int64_t getLength() = 0;

    virtual void setLength(std::int64_t length) = 0;

    virtual void read(std::int64_t offset, ByteBuffer& dest) = 0;

    virtual void write(std::int64_t offset, ByteBuffer& src) = 0;
            
    virtual void flush() = 0;
};
}
