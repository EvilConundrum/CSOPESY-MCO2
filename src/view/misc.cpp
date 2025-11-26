#pragma once
#include <vector>
#include <string>

std::vector<std::string> splitString(const std::string &str, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    for (char ch : str)
    {
        if (ch == delimiter)
        {
            if (!token.empty())
            {
                tokens.push_back(token);
                token.clear();
            }
        }
        else
        {
            token += ch;
        }
    }
    if (!token.empty())
    {
        tokens.push_back(token);
    }
    return tokens;
}

std::string trimString(const std::string &value)
{
    const std::string whitespace = " \t\n\r\f\v";
    size_t start = value.find_first_not_of(whitespace);
    if (start == std::string::npos)
    {
        return "";
    }
    size_t end = value.find_last_not_of(whitespace);
    return value.substr(start, end - start + 1);
}

/**
 * Formats a byte as a two-character hexadecimal
 * string with leading zero if necessary
 */
std::string formatByte(uint8_t byte)
{
    std::stringstream ss;
    if (byte < 16)
        ss << "0"; // leading zero for single digit hex
    ss << std::hex << std::uppercase << static_cast<int>(byte);
    return ss.str();
}

/**
 * Parses a two-character hexadecimal string
 * into its byte (uint8_t) representation
 */

uint8_t parseHexByte(const std::string &hexStr)
{
    return static_cast<uint8_t>(std::stoul(hexStr, nullptr, 16));
}