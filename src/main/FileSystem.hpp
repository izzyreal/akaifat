#pragma once
#include <exception>
#include <memory>

namespace akaifat {

class FsDirectory;

class FileSystem {
public:
    virtual std::shared_ptr<FsDirectory> getRoot() = 0;

    virtual bool isReadOnly() = 0;

    virtual void close() = 0;
    
    virtual bool isClosed() = 0;

    virtual long getTotalSpace() = 0;

    virtual long getFreeSpace() = 0;

    virtual long getUsableSpace() = 0;

    virtual void flush() = 0;
};
}
