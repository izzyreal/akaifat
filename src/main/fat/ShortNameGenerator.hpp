#pragma once

#include <string>
#include <set>
#include <vector>
#include <algorithm>

#include "ShortName.hpp"

#include "../util/string_util.hpp"

namespace akaifat::fat {
class ShortNameGenerator {
private:
	const std::set<std::string>& usedNames;
	
	std::string tidyString(std::string dirty) {
        std::string result;

		for (auto& c : AkaiStrUtil::to_upper_copy(dirty)) {
            if (isSkipChar(c)) continue;

			if (validChar(c)) {
				result.push_back(c);
			} else {
				result.push_back('_');
			}
		}

        return result;
	}

	bool cleanString(std::string s) {
		for (auto& c : s) {
			if (isSkipChar(c)) return false;
			if (!validChar(c)) return false;
		}
		return true;
	}

	std::string stripLeadingPeriods(std::string str) {
		std::string result;

		for (std::int32_t i = 0; i < str.length(); i++) {
			if (str[i] != '.') {
				result += str.substr(i);
				break;
			}
		}

		return result;
	}

public:
	explicit ShortNameGenerator(std::set<std::string>& usedNamesToUse)
    : usedNames(usedNamesToUse)
    {
    }

	static bool validChar(char toTest) {
		if (toTest >= 'A' && toTest <= 'Z') return true;
		if (toTest >= '0' && toTest <= '9') return true;

		return (toTest == '_' || toTest == '^' || toTest == '$' || toTest == '~' || toTest == '!' || toTest == '#'
				|| toTest == '%' || toTest == '&' || toTest == '-' || toTest == '{' || toTest == '}' || toTest == '('
				|| toTest == ')' || toTest == '@' || toTest == '\'' || toTest == '`');
	}

	static bool isSkipChar(char c) {
		return (c == '.') || (c == ' ');
	}

    ShortName generateShortName(std::string longFullName) {
		longFullName = AkaiStrUtil::to_upper_copy(stripLeadingPeriods(longFullName));
		std::string longName;
		std::string longExt;
		auto dotIdx = longFullName.find_last_of('.');
		bool forceSuffix;

		if (dotIdx == -1) {
			forceSuffix = !cleanString(longFullName);
			longName = tidyString(longFullName);
			longExt = "";
		} else {
			forceSuffix = !cleanString(longFullName.substr(0, dotIdx));
			longName = tidyString(longFullName.substr(0, dotIdx));
			longExt = tidyString(longFullName.substr(dotIdx + 1));
		}

		std::string shortExt = (longExt.length() > 3) ? longExt.substr(0, 3) : longExt;

		if (forceSuffix || (longName.length() > 8)
				|| usedNames.find(AkaiStrUtil::to_lower_copy(ShortName(longName, shortExt).asSimpleString())) != usedNames.end()) {

			int maxLongIdx = std::min((int)longName.length(), 8);

			for (int i = 1; i < 99999; i++) {
				std::string serial = "~" + std::to_string(i);
				auto serialLen = serial.length();
				std::string shortName = longName.substr(0, std::min(maxLongIdx, 8 - (int) serialLen)) + serial;

                ShortName result(shortName, shortExt);

				if (usedNames.find(AkaiStrUtil::to_lower_copy(result.asSimpleString())) == usedNames.end()) {
					return result;
				}
			}

			throw std::invalid_argument("could not generate short name for \"" + longFullName + "\"");
		}

		return {longName, shortExt};
    }


};
}
