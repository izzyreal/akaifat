#pragma once
#include <exception>
#include <memory>
#include <cstdint>

namespace akaifat {

class FsDirectory;

class FileSystem {
public:
    virtual std::shared_ptr<FsDirectory> getRoot() = 0;

    virtual bool isReadOnly() = 0;

    virtual void close() = 0;
    
    virtual bool isClosed() = 0;

    virtual std::int64_t getTotalSpace() = 0;

    virtual std::int64_t getFreeSpace() = 0;

    virtual std::int64_t getUsableSpace() = 0;

    virtual void flush() = 0;
};
}
