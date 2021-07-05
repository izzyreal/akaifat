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
//		for (int src = 0; src < dirty.length(); src++) {
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
//		for (int i = 0; i < s.length(); i++) {
//			if (isSkipChar(s.charAt(i))) return false;
//			if (!validChar(s.charAt(i))) return false;
//		}

		return true;
	}

	std::string stripLeadingPeriods(std::string str) {
//		std::stringBuilder sb = new std::stringBuilder(str.length());
//
//		for (int i = 0; i < str.length(); i++) {
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

	ShortName generateShortName(std::string longFullName) {
//		longFullName = stripLeadingPeriods(longFullName).toUpperCase(Locale.ROOT);
//
//		std::string longName;
//		std::string longExt;
//		int dotIdx = longFullName.lastIndexOf('.');
//		bool forceSuffix;
//
//		if (dotIdx == -1) {
//			forceSuffix = !cleanString(longFullName);
//			longName = tidystd::string(longFullName);
//			longExt = "";
//		} else {
//			forceSuffix = !cleanString(longFullName.substring(0, dotIdx));
//			longName = tidystd::string(longFullName.substring(0, dotIdx));
//			longExt = tidystd::string(longFullName.substring(dotIdx + 1));
//		}
//
//		std::string shortExt = (longExt.length() > 3) ? longExt.substring(0, 3) : longExt;
//
//		if (forceSuffix || (longName.length() > 8)
//				|| usedNames.contains(new ShortName(longName, shortExt).asSimpleString().toLowerCase(Locale.ROOT))) {
//
//			int maxLongIdx = Math.min(longName.length(), 8);
//
//			for (int i = 1; i < 99999; i++) {
//				std::string serial = "~" + i; // NOI18N
//				int serialLen = serial.length();
//				std::string shortName = longName.substring(0, Math.min(maxLongIdx, 8 - serialLen)) + serial;
//				ShortName result = new ShortName(shortName, shortExt);
//
//				if (!usedNames.contains(result.asSimpleString().toLowerCase(Locale.ROOT))) {
//
//					return result;
//				}
//			}
//
//			throw "could not generate short name for \"" + longFullName + "\"";
//		}
//
//		return new ShortName(longName, shortExt);
        return ShortName("");
	}

};
}
