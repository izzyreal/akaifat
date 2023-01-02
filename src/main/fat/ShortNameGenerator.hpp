#pragma once

#include <set>
#include <vector>

namespace akaifat::fat {
class ShortNameGenerator {
private:
	std::set<std::string> usedNames;
	
	std::string tidyString(std::string& dirty) {
//		std::stringBuilder result = new std::stringBuilder();
//
//		for (std::int32_t src = 0; src < dirty.length(); src++) {
//			char toTest = Character.toUpperCase(dirty.charAt(src));
//			if (isSkipChar(toTest)) continue;
//
//			if (validChar(toTest)) {
//				result.append(toTest);
//			} else {
//				result.append('_');
//			}
//		}
//
//		return result.tostd::string();
        return "";
	}

	bool cleanString(std::string s) {
//		for (std::int32_t i = 0; i < s.length(); i++) {
//			if (isSkipChar(s.charAt(i))) return false;
//			if (!validChar(s.charAt(i))) return false;
//		}

		return true;
	}

	std::string stripLeadingPeriods(std::string str) {
//		std::stringBuilder sb = new std::stringBuilder(str.length());
//
//		for (std::int32_t i = 0; i < str.length(); i++) {
//			if (str.charAt(i) != '.') { // NOI18N
//				sb.append(str.substring(i));
//				break;
//			}
//		}
//
//		return sb.tostd::string();
        return "";
	}

public:
	ShortNameGenerator(std::set<std::string>& _usedNames)
    : usedNames (_usedNames)
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

	ShortName generateShortName(std::string std::int64_tFullName) {
//		std::int64_tFullName = stripLeadingPeriods(std::int64_tFullName).toUpperCase(Locale.ROOT);
//
//		std::string std::int64_tName;
//		std::string std::int64_tExt;
//		int dotIdx = std::int64_tFullName.lastIndexOf('.');
//		bool forceSuffix;
//
//		if (dotIdx == -1) {
//			forceSuffix = !cleanString(std::int64_tFullName);
//			std::int64_tName = tidystd::string(std::int64_tFullName);
//			std::int64_tExt = "";
//		} else {
//			forceSuffix = !cleanString(std::int64_tFullName.substring(0, dotIdx));
//			std::int64_tName = tidystd::string(std::int64_tFullName.substring(0, dotIdx));
//			std::int64_tExt = tidystd::string(std::int64_tFullName.substring(dotIdx + 1));
//		}
//
//		std::string shortExt = (std::int64_tExt.length() > 3) ? std::int64_tExt.substring(0, 3) : std::int64_tExt;
//
//		if (forceSuffix || (std::int64_tName.length() > 8)
//				|| usedNames.contains(new ShortName(std::int64_tName, shortExt).asSimpleString().toLowerCase(Locale.ROOT))) {
//
//			int maxLongIdx = Math.min(std::int64_tName.length(), 8);
//
//			for (std::int32_t i = 1; i < 99999; i++) {
//				std::string serial = "~" + i; // NOI18N
//				int serialLen = serial.length();
//				std::string shortName = std::int64_tName.substring(0, Math.min(maxLongIdx, 8 - serialLen)) + serial;
//				ShortName result = new ShortName(shortName, shortExt);
//
//				if (!usedNames.contains(result.asSimpleString().toLowerCase(Locale.ROOT))) {
//
//					return result;
//				}
//			}
//
//			throw "could not generate short name for \"" + std::int64_tFullName + "\"";
//		}
//
//		return new ShortName(std::int64_tName, shortExt);
        return ShortName("");
	}

};
}
