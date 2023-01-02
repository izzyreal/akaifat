#pragma once

#include "BlockDevice.hpp"

#include "util/ByteBuffer.hpp"

#include <exception>
#include <fstream>

namespace akaifat {
class ImageBlockDevice : public BlockDevice {
private:
    std::fstream& img;
    std::int64_t mediaSize = -1;
    
public:
    explicit ImageBlockDevice(std::fstream& _img) : img (_img) {}
    explicit ImageBlockDevice(std::fstream& _img, uint64_t _mediaSize) : img (_img), mediaSize (static_cast<std::int64_t>(_mediaSize)) {}
    
    bool isClosed() override { return false; }

    std::int64_t getSize() override {
        
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

    void read(std::int64_t devOffset, ByteBuffer& dest) override {
        if (isClosed()) throw std::runtime_error("device closed");
        
        auto toReadTotal = dest.remaining();

        if ((devOffset + toReadTotal) > getSize())
            throw std::runtime_error("reading past end of device");
        
        if (devOffset % 512 != 0)
        {
            auto offsetWithinSector = devOffset % 512;
            auto sectorOffset = devOffset - offsetWithinSector;
            img.seekg(sectorOffset);
            auto toReadWithStartAlignment = offsetWithinSector + toReadTotal;

            if (toReadWithStartAlignment % 512 != 0)
            {
                auto toReadWithEndAlignment = toReadWithStartAlignment + (512 - (toReadWithStartAlignment % 512));
                if ((sectorOffset + toReadWithEndAlignment) > getSize())
                    throw std::runtime_error("reading past end of device");

                ByteBuffer bb(toReadWithEndAlignment);
                std::vector<char>& buf = bb.getBuffer();
                img.read(&buf[0], toReadWithEndAlignment);
                bb.flip();
                for (std::int32_t i = offsetWithinSector; i < toReadWithStartAlignment; i++)
                    dest.put(buf[i]);
            }
            else
            {
                ByteBuffer bb(toReadWithStartAlignment);
                std::vector<char>& buf = bb.getBuffer();
                img.read(&buf[0], toReadWithStartAlignment);
                bb.flip();
                for (std::int32_t i = offsetWithinSector; i < toReadWithStartAlignment; i++)
                    dest.put(buf[i]);
            }
            return;
        }

        img.seekg(devOffset, std::ios::beg);
        std::vector<char>& buf = dest.getBuffer();

        auto toRead = dest.limit() - dest.position();
        img.read(&buf[0] + dest.position(), toRead);
        dest.position(dest.position() + toRead);
    }

    void write(std::int64_t devOffset, ByteBuffer& src) override {
        if (isClosed()) throw std::runtime_error("device closed");
        
        const auto remaining = src.remaining();

        const auto offsetWithinSector = devOffset % 512;
        const auto sectorOffset = devOffset - offsetWithinSector;
        
        if ((sectorOffset + remaining) > getSize()) throw std::runtime_error("writing past end of device");
                
        auto toWrite = offsetWithinSector + remaining;
        const auto appendSize = 512 - (toWrite % 512);
        toWrite += appendSize;

        ByteBuffer prepend(512);
        read(sectorOffset, prepend);
        prepend.flip();
        
        ByteBuffer append(512);
        read( (sectorOffset + toWrite) - 512, append);
        append.flip();
        
        ByteBuffer data(toWrite);
                
        for (std::int32_t i = 0; i < toWrite; i++)
        {
            if (i < offsetWithinSector) {
                data.put(prepend.get());
            } else if ( (i - offsetWithinSector) < remaining) {
                data.put(src.get());
            } else {
                data.put(append.get());
            }
        }
        
        img.seekp(sectorOffset, std::ios::beg);
        auto& buf = data.getBuffer();
        img.write(&buf[0], toWrite);
    }
            
    void flush() override {
        img.flush();
    }

    std::int32_t getSectorSize() override {
        return 512;
    }

    void close() override {}
    
    bool isReadOnly() override { return false; }
    
};
}
