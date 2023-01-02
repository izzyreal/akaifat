#pragma once

#include "LittleEndian.hpp"

#include "util/string_util.hpp"

#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <stdexcept>

namespace akaifat::fat {
    class AkaiPart {
    private:
        static std::vector<char> &ILLEGAL_CHARS() {
            static std::vector<char> result{
                    0x22, 0x2A, 0x2B, 0x2C, 0x2E, 0x2F, 0x3A, 0x3B,
                    0x3C, 0x3D, 0x3E, 0x3F, 0x5B, 0x5C, 0x5D, 0x7C
            };
            return result;
        }

        static const char ASCII_SPACE = 0x20;

        std::vector<char> nameBytes;

    public:
        static std::vector<std::string> validChars_;
        
        static std::vector<std::string>& validChars() { return AkaiPart::validChars_; }
        
        explicit AkaiPart(std::string akaiPart) {

            if (akaiPart.length() > 8) throw std::runtime_error("Akai part too std::int64_t");

            nameBytes = toCharArray(akaiPart);
            checkValidChars(nameBytes);
        }

        static AkaiPart get(std::string name) {
            return AkaiPart(std::move(name));
        }

        static AkaiPart parse(std::vector<char> &data) {
            std::vector<char> nameArr(8);

            for (std::int32_t i = 0; i < nameArr.size(); i++) {
                nameArr[i] = (char) LittleEndian::getUInt8(data, i + 12);
            }
            std::string akaiPart = std::string(begin(nameArr), end(nameArr));
            if (!isValidAkaiPart(akaiPart)) akaiPart = "        ";
            return AkaiPart(akaiPart);
        }

        void write(std::vector<char> &dest) {
            for (std::int32_t i = 0; i < nameBytes.size(); i++)
                dest[i + 12] = nameBytes[i];
        }

        std::string asSimpleString() {
            auto res = std::string(begin(nameBytes), end(nameBytes));
            return AkaiStrUtil::trim(res);
        }

        static void checkValidChars(const std::vector<char> &chars) {
            for (std::int32_t i = 0; i < chars.size(); i++) {
                if ((chars[i] & 0xff) != chars[i])
                    throw std::runtime_error("multi-byte character at " + std::to_string(i));

                auto toTest = (char) (chars[i] & 0xff);

                if (toTest < 0x20 && toTest != 0x05)
                    throw std::runtime_error("character < 0x20 at" + std::to_string(i));

                for (char j : ILLEGAL_CHARS()) {
                    if (toTest == j)
                        throw std::runtime_error("illegal character " +
                                                 std::to_string(j) + " at " + std::to_string(i));
                }
            }
        }

        static bool isValidAkaiPart(std::string &str) {
            return std::all_of(begin(str), end(str), [](char c){ return isValid(c);});
        }

    private:
        static bool isValid(char c) {

            for (auto s : validChars())
                if (s[0] == c)
                    return true;

            return false;
        }

        static std::vector<char> toCharArray(std::string &name) {
            checkValidName(name);

            std::vector<char> result(8);
            for (std::int32_t i = 0; i < 8; i++)
                result[i] = ASCII_SPACE;

            for (std::int32_t i = 0; i < name.length(); i++)
                result[i] = name[i];

            return result;
        }

        static void checkValidName(std::string &name) {
            checkString(name, "name", 0, 8);
        }

        static void checkString(std::string &str, const std::string& strType,
                                std::int32_t minLength, std::int32_t maxLength) {
            if (str.length() < minLength)
                throw std::runtime_error(strType + " must have at least " + std::to_string(minLength) +
                                         " characters: " + str);
            if (str.length() > maxLength)
                throw std::runtime_error(strType +
                                         " has more than " + std::to_string(maxLength) +
                                         " characters: " + str);
        }
    };
}
