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