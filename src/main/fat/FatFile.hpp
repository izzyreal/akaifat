#pragma once

#include "../AbstractFsObject.hpp"
#include "../FsFile.hpp"

#include "Fat.hpp"
#include "ClusterChain.hpp"
#include "FatDirectoryEntry.hpp"

#include <exception>
#include <utility>
#include <iostream>

namespace akaifat::fat {

class FatFile : public akaifat::AbstractFsObject, public akaifat::FsFile {
private:
    std::shared_ptr<FatDirectoryEntry> entry;
    ClusterChain chain;
    std::streambuf* ibuf = nullptr;
    std::streambuf* obuf = nullptr;

public:
    ~FatFile() { if (obuf != nullptr) delete obuf; if (ibuf != nullptr) delete ibuf; }
    FatFile(const std::shared_ptr<FatDirectoryEntry>& myEntry, ClusterChain _chain)
    : akaifat::AbstractFsObject(myEntry->isReadOnly()), entry(myEntry), chain(std::move(_chain)) {}
    
    static std::shared_ptr<FatFile> get(Fat *fat, const std::shared_ptr<FatDirectoryEntry>& entry) {
        
        if (entry->isDirectory())
            throw std::runtime_error(entry->getShortName().asSimpleString() + " is a directory");
        
        ClusterChain cc(
                        fat, entry->getStartCluster(), entry->isReadonlyFlag());
        
        if (entry->getLength() > cc.getLengthOnDisk())
            throw std::runtime_error("entry (" + std::to_string(entry->getLength()) +
                                     ") is larger than associated cluster chain ("
                                     + std::to_string(cc.getLengthOnDisk()) + ")");
        
        return std::make_shared<FatFile>(entry, cc);
    }
    
    long getLength() override {
        checkValid();
        
        return entry->getLength();
    }
    
    void setLength(long length) override {
        checkWritable();
        
        if (getLength() == length) return;
        
        chain.setSize(length);
        
        entry->setStartCluster(chain.getStartCluster());
        entry->setLength(length);
    }
    
    void read(long offset, ByteBuffer &dest) override {
        checkValid();
        
        auto len = dest.remaining();
        
        if (len == 0) return;
        
        if (offset + len > getLength())
            throw std::runtime_error("EOF");
        
        chain.readData(offset, dest);
    }
    
    void write(long offset, ByteBuffer &srcBuf) override {
        
        checkWritable();
        
        long lastByte = offset + srcBuf.remaining();
        
        if (lastByte > getLength())
            setLength(lastByte);
        
        chain.writeData(offset, srcBuf);
    }
    
    void flush() override {
        checkWritable();
    }
    
    ClusterChain& getChain() {
        checkValid();
        
        return chain;
    }
    
    std::shared_ptr<std::istream> getInputStream() {
        
        class akai_streambuf : public std::streambuf
        {
        private:
            FatFile* fatFile;
            std::streampos pos = 0;
        public:
            akai_streambuf(FatFile* _fatFile) : fatFile (_fatFile) {}
        protected:
            std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode = std::ios_base::in) override
            {
                if (way == std::ios_base::beg)
                    pos = off;
                else if (way == std::ios_base::cur)
                    pos += off;
                else if (way == std::ios_base::end)
                    pos = fatFile->getLength() + off;
                return pos;
            }
            std::streamsize xsgetn (char* s, std::streamsize n) override
            {
                ByteBuffer bb(n);
                fatFile->read(pos, bb);
                auto& buf = bb.getBuffer();
                for (size_t i=0;i<n;i++) s[i]=buf[i];
                pos += n;
                return n;
            }
        };
        
        if (ibuf != nullptr)
            delete ibuf;
        
        ibuf = new akai_streambuf(this);
        
        return std::make_shared<std::istream>(ibuf);
    }
    
    std::unique_ptr<std::ostream> getOutputStream() {
        class akai_streambuf : public std::streambuf
        {
        private:
            std::streampos pos = 0;
            FatFile* fatFile;
            ByteBuffer bb = ByteBuffer(0);
        public:
            akai_streambuf(FatFile* _fatFile) : fatFile (_fatFile) {}
        protected:
            std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode = std::ios_base::out) override
            {
                if (way == std::ios_base::beg)
                    pos = off;
                else if (way == std::ios_base::cur)
                    pos += off;
                else if (way == std::ios_base::end)
                    pos = fatFile->getLength() + off;
                return pos;
            }
            std::streamsize xsputn (const char* s, std::streamsize n) override
            {
                bb.clearAndAllocate(n);
                auto& buf = bb.getBuffer();
                for (size_t i=0;i<n;i++) buf[i]=s[i];
                fatFile->write(pos, bb);
                pos += n;
                return n;
            }
        };
        
        if (obuf != nullptr)
            delete obuf;
        
        obuf = new akai_streambuf(this);
        
        return std::make_unique<std::ostream>(ibuf);
    }
};
}
