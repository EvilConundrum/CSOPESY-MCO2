#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <cstdint>
#include <functional>
#include "Instruction.cpp"
#include "../view/misc.cpp"

#ifndef BASH_DELIMITER
#define BASH_DELIMITER ';'
#endif

class InstructionParser
{
    std::vector<std::string> lines;
    // adds to this when a new variable is declared
    // checks against this when variables are used in instructions
    std::vector<std::string> variables;
    std::vector<Instruction> instructions;
    bool isValid = true;

public:
    InstructionParser() {}
    InstructionParser(const std::string &input)
    {
        // separate lines
        splitLines(input);
        // tokenize each line
        for (const auto &line : lines)
        {
            tokenizeLine(line);
        }
    }

    /**
     * Parses the raw input into instructions.
     * returns whether input instructions are valid.
     */
    bool parse()
    {
        for (size_t i = 0; i < lines.size(); ++i)
        {
            std::vector<std::string> tokens = tokenizeLine(lines[i]);
        }
        return isValid;
    }

    bool getValidity() const
    {
        return isValid;
    }

private:
    bool isInVariables(const std::string &var)
    {
        return std::find(variables.begin(), variables.end(), var) != variables.end();
    }

    void splitLines(const std::string &input)
    {
        std::istringstream stream(input);
        std::string line;

        while (std::getline(stream, line, BASH_DELIMITER))
        {
            // remove leading/trailing whitespace
            line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
            line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);

            if (!line.empty())
                lines.push_back(line);
        }
    }

    std::vector<std::string> tokenizeLine(const std::string &line)
    {
        std::istringstream stream(line);
        std::string token;
        std::vector<std::string> tokens;

        if (line.find("PRINT") != std::string::npos)
        {
            tokens.push_back("PRINT");
        }
        else
        {
            tokens = splitString(line, ' ');
            return tokens;
        }
    }

    bool verifyPrint(const std::string &line)
    {
        std::string s = line;

        // checks if line starts with PRINT( and ends with )
        if (s.rfind("PRINT(", 0) != 0 || s.back() != ')')
            return false;

        // checks if there is at least one character inside the parentheses
        std::string inner = s.substr(6, s.size() - 7);
        if (inner.empty())
            return false;
        return true;
    }

    bool verifyDeclare(const std::vector<std::string> &tokens)
    {
        // DECLARE should have exactly 2 arguments
        if (tokens.size() != 3)
            return false;

        // variable name should not already exist
        if (isInVariables(tokens[1]))
            return false;

        return true;
    }

    bool verifyArithmetic(const std::vector<std::string> &tokens, const std::string &op)
    {
        const auto isNumeric = [](const std::string &s)
        {
            return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
        };

        // ADD or SUBTRACT should have exactly 3 arguments
        if (tokens.size() != 4)
            return false;

        if (!isInVariables(tokens[1]))
            return false; // destination variable must be declared

        if ((!isInVariables(tokens[2]) && !isNumeric(tokens[2])) ||
            (!isInVariables(tokens[3]) && !isNumeric(tokens[3])))
            return false; // operands must be a declared variable or numeric

        return true;
    }

    bool verifySleep(const std::vector<std::string> &tokens)
    {
        const auto isNumeric = [](const std::string &s)
        {
            return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
        };

        // SLEEP should have exactly 1 argument
        if (tokens.size() != 2)
            return false;

        if (!isNumeric(tokens[1]))
            return false; // argument must be numeric

        return true;
    }
};