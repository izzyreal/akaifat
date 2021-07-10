#pragma once

#include <string>

namespace akaifat {

    class StrUtil {
    public:
        static inline std::string trim_copy(std::string s, const char *t = " \t\n\r\f\v") {
            return trim(s, t);
        }

        static std::string trim(const std::string &str,
                                const std::string &whitespace = " \t\0") {
            const auto strBegin = str.find_first_not_of(whitespace);
            if (strBegin == std::string::npos)
                return ""; // no content

            const auto strEnd = str.find_last_not_of(whitespace);
            const auto strRange = strEnd - strBegin + 1;

            return str.substr(strBegin, strRange);
        }

        static inline std::string to_upper_copy(std::string s) {
            for (int i = 0; i < s.length(); i++)
                s[i] = toupper(s[i]);
            return s;
        }

        static inline std::string to_lower_copy(std::string s) {
            for (int i = 0; i < s.length(); i++)
                s[i] = tolower(s[i]);
            return s;
        }
    };

}
