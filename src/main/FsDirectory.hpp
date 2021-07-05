#pragma once

#include "FsDirectoryEntry.hpp"

#include <memory>
#include <algorithm>
#include <exception>
#include <map>
#include <iterator>
#include <string>

namespace akaifat {
class FsDirectory {
public:
    virtual std::shared_ptr<FsDirectoryEntry> getEntry(std::string& name) = 0;

    virtual std::shared_ptr<FsDirectoryEntry> addFile(std::string& name) = 0;

    virtual std::shared_ptr<FsDirectoryEntry> addDirectory(std::string& name) = 0;

    virtual void remove(std::string& name) = 0;

    virtual void flush() = 0;

    virtual bool isDirValid() = 0;
    
    virtual bool isDirReadOnly() = 0;
    
};
}
