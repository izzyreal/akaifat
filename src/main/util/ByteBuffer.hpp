#pragma once

#include "../fat/LittleEndian.hpp"

#include <vector>
#include <stdexcept>

namespace akaifat {
class ByteBuffer {
private:
    std::vector<char> buf;
    std::int64_t pos = 0;
    std::int64_t limit_ = 0;
    
public:
    ByteBuffer(std::int64_t size)
    : buf (std::vector<char>(size)), limit_ (size) {}
    ByteBuffer(std::vector<char>& data) : buf (data), limit_ (data.size()) {}

    void clearAndAllocate(std::int64_t newSize) { buf.clear(); pos = 0; limit_ = newSize; buf.resize(newSize); }
    
    void flip() { limit_ = pos; pos = 0; }
    
    void get(std::vector<char>& dest) {
        for (std::int32_t i = 0; i < dest.size(); i++)
        {
            if (i >= buf.size()) throw std::runtime_error("invalid bytebuffer read");
            dest[i] = get();
        }
    }
    
    char get() { return buf[pos++]; }
    char get(std::int64_t index) { return buf[index]; }

    std::uint32_t getInt() {
        return getInt(pos);
    }
    
    std::uint32_t getInt(std::int64_t index) {
        std::uint32_t result = 0;
        for (int i = 0; i < 4; ++i)
            result |= static_cast<std::uint32_t>(static_cast<unsigned char>(buf[index + i])) << (i * 8);
        pos += 4;
        return result;
    }
    
    short getShort() {
        return getShort(pos);
    }
    
    short getShort(std::int64_t index) {
        const auto bits = static_cast<unsigned char>(buf[index]) |
                          (static_cast<unsigned char>(buf[index + 1]) << 8);
        const auto result = bits >= 0x8000 ? bits - 0x10000 : bits;
        pos += 2;
        return static_cast<short>(result);
    }
    
    std::int64_t position() { return pos; }
    void position(std::int64_t newPos) { pos = newPos; }
    std::int64_t remaining() { return limit_ - pos; }
    bool hasRemaining() { return pos < limit_; }
    void rewind() { pos = 0; }
    
    void limit(std::int64_t newLimit) {
        if (newLimit > buf.size() || newLimit < 0)
            throw new std::runtime_error("Invalid limit");
        
        limit_ = newLimit;
        if (pos > limit_) pos = limit_;
    }

    std::int64_t limit() { return limit_; }

    void put(char c) { buf[pos++] = c; }
    void put(std::int32_t offset, char c) { buf[offset] = c; }

    void put(std::vector<char>& data) {
        for (std::int32_t i = 0; i < data.size(); i++) {
            if (i >= buf.size()) throw std::runtime_error("invalid bytebuffer write");
            buf[pos++] = data[i];
        }
    }
    
    std::vector<char>& getBuffer() { return buf; }
    std::int64_t capacity() { return buf.size(); }

    void putShort(std::int32_t offset, short s) {
        fat::LittleEndian::setInt16(buf, offset, s);
    }

    void putInt(std::int32_t offset, std::int32_t value)
    {
        fat::LittleEndian::setInt32(buf, offset, value);
    }
};
}
