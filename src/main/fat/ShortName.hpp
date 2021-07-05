#pragma once

#include "../strutil.hpp"

#include "LittleEndian.hpp"

#include <string>
#include <vector>

namespace akaifat::fat {
    class ShortName {
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

        static std::vector<char> toCharArray(std::string &name, std::string &ext) {
        checkValidName(name);
        checkValidExt(ext);

            std::vector<char> result(11);
            for (int i = 0; i < 11; i++)
                result[i] = ASCII_SPACE;

            for (int i = 0; i < name.length(); i++)
                result[i] = name[i];

            for (int i = 8; i < 8 + ext.length(); i++)
                result[i] = ext[i - 8];

            return result;
        }

        static void checkValidName(std::string &name) {
            checkString(name, "name", 1, 8);
        }

        static void checkValidExt(std::string &ext) {
            checkString(ext, "extension", 0, 3);
        }

        static void checkString(std::string &str, const std::string& strType,
                                int minLength, int maxLength) {

            if (str.length() < minLength)
                throw std::runtime_error(strType +
                      " must have at least " + std::to_string(minLength) +
                      " characters: " + str);
            if (str.length() > maxLength)
                throw std::runtime_error(strType +
                      " has more than " + std::to_string(maxLength) +
                      " characters: " + str);
        }

    public:
        ShortName() = default;

        explicit ShortName(std::string &nameExt) {
            if (nameExt.length() > 12) throw std::runtime_error("name too long");

            auto i = nameExt.find_last_of('.');

            std::string name;
            std::string ext;

            if (i == std::string::npos) {
                name = StrUtil::to_upper_copy(nameExt);
                ext = "";
            } else {
                name = StrUtil::to_upper_copy(nameExt.substr(0, i));
                ext = StrUtil::to_upper_copy(nameExt.substr(i + 1));
            }

            nameBytes = toCharArray(name, ext);
            checkValidChars(nameBytes);
        }

        ShortName(std::string name, std::string ext) {
            nameBytes = toCharArray(name, ext);
        }

        static ShortName &DOT() {
            static ShortName dot(".", "");
            return dot;
        };

        static ShortName &DOT_DOT() {
            static ShortName dotDot("..", "");
            return dotDot;
        }

        static ShortName get(std::string &name) {
            if (name == ".") return DOT();
            else if (name == "..") return DOT_DOT();
            else return ShortName(name);
        }

        static bool canConvert(std::string nameExt) {
            try {
                ShortName::get(nameExt);
                return true;
            } catch (std::exception &) {
                return false;
            }
        }

        static ShortName parse(std::vector<char> &data) {
            std::string name;

            for (int i = 0; i < 8; i++)
                name.push_back((char) LittleEndian::getUInt8(data, i));

            if (LittleEndian::getUInt8(data, 0) == 0x05) {
                name[0] = (char) 0xe5;
            }

            std::string ext;

            for (int i = 0; i < 3; i++)
                ext.push_back((char) LittleEndian::getUInt8(data, 0x08 + i));

            StrUtil::trim(name, " ");
            StrUtil::trim(ext, " ");

            return ShortName(name, ext);
        }

        void write(std::vector<char> &dest) {
            for (int i = 0; i < nameBytes.size(); i++)
                dest[i] = nameBytes[i];
        }

        std::string asSimpleString() {
            std::string name;

            for (int i = 0; i < 8; i++) {
                char c = (char) LittleEndian::getUInt8(nameBytes, i);
                if (c == 0x00) continue;
                name.push_back(c);
            }

            if (LittleEndian::getUInt8(nameBytes, 0) == 0x05) {
                name[0] = (char) 0xe5;
            }

            std::string ext;

            for (int i = 0; i < 3; i++) {
                char c = (char) LittleEndian::getUInt8(nameBytes, 0x08 + i);
                if (c == 0x00) continue;
                ext.push_back(c);
            }

            name = StrUtil::trim(name);
            ext = StrUtil::trim(ext);

            return ext.length() == 0 ? name : (name + "." + ext);
        }

        static void checkValidChars(std::vector<char> &chars) {

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

        bool equals(const ShortName &other) {
            if (nameBytes.size() != other.nameBytes.size()) return false;

            for (int i = 0; i < nameBytes.size(); i++)
                if (nameBytes[i] != other.nameBytes[i]) return false;

            return true;
        }
    };
}
