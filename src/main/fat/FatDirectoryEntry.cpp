#include "FatDirectoryEntry.hpp"

#include "AkaiFatLfnDirectory.hpp"
#include "AkaiPart.hpp"

using namespace akaifat;
using namespace akaifat::fat;

void FatDirectoryEntry::setAkaiName(std::string s) {
    	std::string part1 = AkaiFatLfnDirectory::splitName(s)[0];
    	std::string part2 = "        ";
    	std::string ext = AkaiFatLfnDirectory::splitName(s)[1];
    	if (part1.length() > 8) {
    		part2 = part1.substr(8);
    		part1 = part1.substr(0, 8);
    	}
    	if (ext.length() > 0) ext = "."+ ext;
    	std::string name = std::string(part1 + ext);
    	ShortName sn(name);
    	sn.write(data);
    	AkaiPart ap(part2);
    	ap.write(data);
}
