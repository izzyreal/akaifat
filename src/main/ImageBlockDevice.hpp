#pragma once

#include "BlockDevice.hpp"

#include "util/ByteBuffer.hpp"

#include <exception>
#include <fstream>

namespace akaifat {
class ImageBlockDevice : public BlockDevice {
private:
    std::fstream& img;
    long mediaSize = -1;
    
public:
    explicit ImageBlockDevice(std::fstream& _img) : img (_img) {}
    explicit ImageBlockDevice(std::fstream& _img, uint64_t _mediaSize) : img (_img), mediaSize (static_cast<long>(_mediaSize)) {}
    
    bool isClosed() override { return false; }

    long getSize() override {
        
        if (mediaSize != -1) {
            return mediaSize;
        }
        
        img.seekg(0);
        const auto begin = img.tellg();
        img.seekg (0, std::ios::end);
        const auto end = img.tellg();
        const auto fsize = (end-begin);
        return fsize;
    }

    void read(long devOffset, ByteBuffer& dest) override {
        if (isClosed()) throw std::runtime_error("device closed");
        
        auto toReadTotal = dest.remaining();

        if ((devOffset + toReadTotal) > getSize())
            throw std::runtime_error("reading past end of device");
        
        img.seekg(devOffset);
        std::vector<char>& buf = dest.getBuffer();

        auto toRead = dest.limit() - dest.position();
        img.read(&buf[0] + dest.position(), toRead);
        dest.position(dest.position() + toRead);
    }

    void write(long devOffset, ByteBuffer& src) override {
        if (isClosed()) throw std::runtime_error("device closed");

        auto toWriteTotal = src.remaining();
        if ((devOffset + toWriteTotal) > getSize()) throw std::runtime_error("writing past end of device");

        img.seekp(devOffset);
        auto toWrite = src.limit() - src.position();

        if (toWrite % 512 != 0)
        {
            int padSize = 512 - (toWrite % 512);
            int firstPos = (devOffset + toWrite + padSize) - 512;
            ByteBuffer onDisk(512);
            read(firstPos, onDisk);
            
            for (int i = toWrite; i < toWrite + padSize; i++)
                src.put(i, onDisk.getBuffer()[i % 512]);

            toWrite += padSize;
        }

        img.seekp(devOffset);

        std::vector<char>& buf = src.getBuffer();
        img.write(&buf[0] + src.position(), toWrite);
        src.position(src.position() + toWrite);
    }
            
    void flush() override {
    }

    int getSectorSize() override {
        return 512;
    }

    void close() override {}
    
    bool isReadOnly() override { return false; }
    
};
}
