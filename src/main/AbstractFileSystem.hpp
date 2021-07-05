#pragma once

#include "FileSystem.hpp"

namespace akaifat {
class AbstractFileSystem : public FileSystem {
private:
    bool readOnly;
    bool closed;
    
public:
    AbstractFileSystem(bool _readOnly)
    : readOnly (_readOnly){
        closed = false;
    }
    
    void close() override {
        if (!isClosed()) {
            if (!isReadOnly()) {
                flush();
            }
            
            closed = true;
        }
    }
    
    bool isClosed() override {
        return closed;
    }
    
    bool isReadOnly() override {
        return readOnly;
    }

protected:
    void checkClosed() {
        if (isClosed()) {
            throw "file system was already closed";
        }
    }
    
    void checkReadOnly() {
        if (isReadOnly()) {
            throw "file system is read only";
        }
    }
};
}
