#pragma once

#include <vector>
#include <stdexcept>

namespace akaifat {
class ByteBuffer {
private:
    std::vector<char> buf;
    long pos = 0;
    long limit_ = 0;
    
public:
    ByteBuffer(long size)
    : buf (std::vector<char>(size)), limit_ (size) {}
    ByteBuffer(std::vector<char>& data) : buf (data), limit_ (data.size()) {}

    void clearAndAllocate(long newSize) { buf.clear(); pos = 0; limit_ = newSize; buf.resize(newSize); }
    
    void flip() { limit_ = pos; pos = 0; }
    
    void get(std::vector<char>& dest) {
        for (int i = 0; i < dest.size(); i++)
        {
            if (i >= buf.size()) throw std::runtime_error("invalid bytebuffer read");
            dest[i] = get();
        }
    }
    
    char get() { return buf[pos++]; }
    char get(long index) { return buf[index]; }

    unsigned int getInt() {
        return getInt(pos);
    }
    
    unsigned int getInt(long index) {
        char chars[4];
        for (int i = 0; i < 4; i++)
            chars[i] = buf[index + i];
        pos += 4;
        return *(unsigned int *) chars;
    }
    
    short getShort() {
        return getShort(pos);
    }
    
    short getShort(long index) {
        short result = (buf[index] & 0xff) | (buf[index + 1] << 8);
        pos += 2;
        return result;
    }
    
    long position() { return pos; }
    void position(long newPos) { pos = newPos; }
    long remaining() { return limit_ - pos; }
    bool hasRemaining() { return pos < limit_; }
    void rewind() { pos = 0; }
    
    void limit(long newLimit) {
        if (newLimit > buf.size() || newLimit < 0)
            throw new std::runtime_error("Invalid limit");
        
        limit_ = newLimit;
        if (pos > limit_) pos = limit_;
    }

    long limit() { return limit_; }

    void put(char c) { buf[pos++] = c; }
    void put(int offset, char c) { buf[offset] = c; }

    void put(std::vector<char>& data) {
        for (int i = 0; i < data.size(); i++) {
            if (i >= buf.size()) throw std::runtime_error("invalid bytebuffer write");
            buf[pos++] = data[i];
        }
    }
    
    std::vector<char>& getBuffer() { return buf; }
    long capacity() { return buf.size(); }

    void putShort(int offset, short s) {
        buf[offset + 1] = (s >> 8);
        buf[offset] = s & 0xFF;
    }
};
}
