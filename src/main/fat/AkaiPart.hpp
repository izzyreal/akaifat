#pragma once

#include <string>
#include <utility>
#include <vector>

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
        static std::vector<std::string> &validChars() {
            static std::vector<std::string> result{" ", "!", "#", "$", "%", "&",
                                                   "'", "(", ")", "-", "0", "1", "2", "3", "4", "5", "6", "7", "8",
                                                   "9", "@", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K",
                                                   "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X",
                                                   "Y", "Z", "_", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j",
                                                   "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w",
                                                   "x", "y", "z", "{", "}", "~"};
            return result;
        }

        explicit AkaiPart(std::string akaiPart) {

            if (akaiPart.length() > 8) throw std::runtime_error("Akai part too long");

            nameBytes = toCharArray(akaiPart);
//            checkValidChars(nameBytes);
        }

        static AkaiPart get(std::string name) {
            return AkaiPart(std::move(name));
        }

        static AkaiPart parse(std::vector<char> &data) {
            std::vector<char> nameArr(8);

            for (int i = 0; i < nameArr.size(); i++) {
                nameArr[i] = (char) LittleEndian::getUInt8(data, i + 12);
            }
            std::string akaiPart = std::string(begin(nameArr), end(nameArr));
            if (!isValidAkaiPart(akaiPart)) akaiPart = "        ";
            return AkaiPart(akaiPart);
        }

        void write(std::vector<char> &dest) {
            for (int i = 0; i < nameBytes.size(); i++)
                dest[i + 12] = nameBytes[i];
        }

        std::string asSimpleString() {
            return std::string(begin(nameBytes), end(nameBytes));
        }

        static void checkValidChars(const std::vector<char> &chars) {

            if (chars[0] == 0x20) throw std::runtime_error("0x20 can not be the first character");

            for (int i = 0; i < chars.size(); i++) {
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
            for (int i = 0; i < 8; i++)
                result[i] = ASCII_SPACE;

            for (int i = 0; i < name.length(); i++)
                result[i] = name[i];

            return result;
        }

        static void checkValidName(std::string &name) {
            checkString(name, "name", 0, 8);
        }

        static void checkString(std::string &str, const std::string& strType,
                                int minLength, int maxLength) {
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
