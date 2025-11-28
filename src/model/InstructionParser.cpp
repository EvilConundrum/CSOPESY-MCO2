#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <cstdint>
#include <functional>
#include <algorithm>
#include <cctype>
#include "Instruction.cpp"
#include "../view/misc.cpp"

#ifndef BASH_DELIMITER
#define BASH_DELIMITER ';'
#endif

class InstructionParser
{
    std::vector<std::string> lines;
    std::vector<std::string> variables;
    std::vector<Instruction> instructions;
    bool isValid = true;

public:
    InstructionParser() {}

    InstructionParser(const std::string &input)
    {
        splitLines(input);
    }

    bool parse()
    {
        instructions.clear();
        variables.clear();
        isValid = true;

        for (const auto &line : lines)
        {
            if (!parseLine(line))
            {
                isValid = false;
                break;
            }
        }
        return isValid;
    }

    bool getValidity() const
    {
        return isValid;
    }

    const std::vector<Instruction> &getInstructions() const
    {
        return instructions;
    }

    const std::vector<std::string> &getLines() const
    {
        return lines;
    }

private:
    void splitLines(const std::string &input)
    {
        lines.clear();
        std::istringstream stream(input);
        std::string line;

        while (std::getline(stream, line, BASH_DELIMITER))
        {
            std::string trimmed = trimString(line);
            if (!trimmed.empty())
            {
                lines.push_back(trimmed);
            }
        }
    }

    bool parseLine(const std::string &line)
    {
        std::string trimmed = trimString(line);
        if (trimmed.empty())
        {
            return true;
        }

        if (startsWithIgnoreCase(trimmed, "PRINT("))
        {
            return handlePrint(trimmed);
        }

        auto tokens = tokenizeArguments(trimmed);
        if (tokens.empty())
        {
            return true;
        }

        std::string command = toUpper(tokens[0]);
        std::vector<std::string> args(tokens.begin() + 1, tokens.end());
        bool handled = false;

        if (command == "DECLARE")
        {
            if (!handleDeclare(tokens))
                return false;
            handled = true;
        }
        else if (command == "ADD" || command == "SUBTRACT")
        {
            if (!handleArithmetic(tokens))
                return false;
            handled = true;
        }
        else if (command == "SLEEP")
        {
            if (!handleSleep(tokens))
                return false;
            handled = true;
        }
        else if (command == "PRINT")
        {
            if (!handlePrint(trimmed))
                return false;
            return true; // already pushed instruction inside handlePrint
        }

        if (!handled)
        {
            instructions.emplace_back(command, args);
        }
        return true;
    }

    bool handleDeclare(const std::vector<std::string> &tokens)
    {
        if (tokens.size() != 3)
        {
            std::cerr << "[Parser] DECLARE expects 2 arguments" << std::endl;
            return false;
        }

        const std::string &var = tokens[1];
        if (isInVariables(var))
        {
            std::cerr << "[Parser] Variable '" << var << "' already declared" << std::endl;
            return false;
        }

        variables.push_back(var);
        instructions.emplace_back("DECLARE", std::vector<std::string>{tokens[1], tokens[2]});
        return true;
    }

    bool handleArithmetic(const std::vector<std::string> &tokens)
    {
        if (tokens.size() != 4)
        {
            std::cerr << "[Parser] Arithmetic instructions expect 3 arguments" << std::endl;
            return false;
        }

        if (!isInVariables(tokens[1]))
        {
            std::cerr << "[Parser] Variable '" << tokens[1] << "' must be declared before use" << std::endl;
            return false;
        }

        if (!isValue(tokens[2]) || !isValue(tokens[3]))
        {
            std::cerr << "[Parser] Arithmetic operands must be declared variables or numeric" << std::endl;
            return false;
        }

        instructions.emplace_back(tokens[0], std::vector<std::string>{tokens[1], tokens[2], tokens[3]});
        return true;
    }

    bool handleSleep(const std::vector<std::string> &tokens)
    {
        if (tokens.size() != 2 || !isNumber(tokens[1]))
        {
            std::cerr << "[Parser] SLEEP expects numeric tick count" << std::endl;
            return false;
        }

        instructions.emplace_back("SLEEP", std::vector<std::string>{tokens[1]});
        return true;
    }

    bool handlePrint(const std::string &line)
    {
        size_t openParen = line.find('(');
        size_t closeParen = line.rfind(')');

        if (openParen == std::string::npos || closeParen == std::string::npos || closeParen <= openParen + 1)
        {
            std::cerr << "[Parser] PRINT expects parentheses" << std::endl;
            return false;
        }

        std::string inner = line.substr(openParen + 1, closeParen - openParen - 1);
        auto args = parsePrintArguments(inner);

        if (args.empty())
        {
            std::cerr << "[Parser] PRINT requires at least one argument" << std::endl;
            return false;
        }

        instructions.emplace_back("PRINT", args);
        return true;
    }

    bool isInVariables(const std::string &var) const
    {
        return std::find(variables.begin(), variables.end(), var) != variables.end();
    }

    bool isValue(const std::string &token) const
    {
        return isInVariables(token) || isNumber(token);
    }

    bool isNumber(const std::string &token) const
    {
        if (token.empty())
            return false;

        return std::all_of(token.begin(), token.end(), [](unsigned char ch)
                           { return std::isdigit(ch); });
    }

    std::vector<std::string> tokenizeArguments(const std::string &line)
    {
        std::vector<std::string> result;
        std::string current;
        bool inQuotes = false;

        for (char ch : line)
        {
            if (ch == '"')
            {
                inQuotes = !inQuotes;
                current += ch;
                continue;
            }

            if (!inQuotes && std::isspace(static_cast<unsigned char>(ch)))
            {
                if (!current.empty())
                {
                    result.push_back(current);
                    current.clear();
                }
                continue;
            }

            current += ch;
        }

        if (!current.empty())
        {
            result.push_back(current);
        }

        return result;
    }

    std::vector<std::string> parsePrintArguments(const std::string &inner)
    {
        std::vector<std::string> args;
        std::string current;
        bool inQuotes = false;

        auto flushCurrent = [&]()
        {
            std::string trimmed = trimString(current);
            if (!trimmed.empty())
            {
                args.push_back(trimmed);
            }
            current.clear();
        };

        for (char ch : inner)
        {
            if (ch == '"')
            {
                inQuotes = !inQuotes;
                current += ch;
                continue;
            }

            if (!inQuotes && ch == '+')
            {
                flushCurrent();
                args.push_back("+");
                continue;
            }

            current += ch;
        }

        flushCurrent();

        // Remove trailing '+' if present
        if (!args.empty() && args.back() == "+")
        {
            args.pop_back();
        }

        // Remove duplicate '+' tokens
        std::vector<std::string> normalized;
        bool lastPlus = false;
        for (const auto &token : args)
        {
            if (token == "+")
            {
                if (!lastPlus && !normalized.empty())
                {
                    normalized.push_back(token);
                }
                lastPlus = true;
            }
            else
            {
                normalized.push_back(token);
                lastPlus = false;
            }
        }

        return normalized.empty() ? args : normalized;
    }

    std::string toUpper(const std::string &value) const
    {
        std::string result = value;
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch)
                       { return static_cast<char>(std::toupper(ch)); });
        return result;
    }

    bool startsWithIgnoreCase(const std::string &value, const std::string &prefix) const
    {
        if (value.size() < prefix.size())
        {
            return false;
        }

        for (size_t i = 0; i < prefix.size(); ++i)
        {
            if (std::toupper(static_cast<unsigned char>(value[i])) != std::toupper(static_cast<unsigned char>(prefix[i])))
            {
                return false;
            }
        }
        return true;
    }
};