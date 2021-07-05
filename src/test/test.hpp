#include "fat/AkaiFatLfnDirectory.hpp"
#include "fat/AkaiFatFileSystem.hpp"

#include <fstream>

class AkaiFatTestsFixture {
private:
    static int uniqueID;
    std::fstream img;
protected:
    std::shared_ptr<akaifat::BlockDevice> device;
    std::shared_ptr<akaifat::fat::AkaiFatLfnDirectory> root;
    akaifat::fat::AkaiFatFileSystem* fs;
public:
    AkaiFatTestsFixture();
    ~AkaiFatTestsFixture();
protected:
    int getID() {
        return ++uniqueID;
    }
};
