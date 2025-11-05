#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstdint>

enum class InstructionType {
    PRINT,
    DECLARE,
    ADD,
    SUBTRACT,
    SLEEP,
    FOR,
    UNKNOWN
};

class Instruction {
    InstructionType type;
    std::string command;
    std::vector<std::string> args;
    int lineNumber;
    std::vector<Instruction> subInstructions;

public:
    // Constructors
    Instruction() : type(InstructionType::UNKNOWN), lineNumber(0) {}

    Instruction(InstructionType type, int lineNumber)
        : type(type), lineNumber(lineNumber) {}

    Instruction(const std::string &command, const std::vector<std::string> &arguments)
        : command(command), args(arguments), lineNumber(0) {
        std::string cmd = command;
        for (auto &c : cmd) c = std::toupper(c);

        if (cmd == "PRINT") type = InstructionType::PRINT;
        else if (cmd == "DECLARE") type = InstructionType::DECLARE;
        else if (cmd == "ADD") type = InstructionType::ADD;
        else if (cmd == "SUBTRACT") type = InstructionType::SUBTRACT;
        else if (cmd == "SLEEP") type = InstructionType::SLEEP;
        else if (cmd == "FOR") type = InstructionType::FOR;
        else type = InstructionType::UNKNOWN;
    }

    void setSubInstructions(const std::vector<Instruction> &subs) {
        subInstructions = subs;
    }

    InstructionType getType() const { return type; }
    std::string getCommand() const { return command; }
    std::vector<std::string> getArgs() const { return args; }
    int getLineNumber() const { return lineNumber; }
    bool isValid() const { return type != InstructionType::UNKNOWN; }

    /**
     * Executes the instruction using the provided variable map.
     * Returns number of CPU ticks consumed (0 for instant, >0 for sleep).
     */
    int execute(std::unordered_map<std::string, uint16_t> &variables) const {
        switch (type) {
            case InstructionType::PRINT:
                executePrint(variables);
                return 0;
            case InstructionType::DECLARE:
                executeDeclare(variables);
                return 0;
            case InstructionType::ADD:
                executeAdd(variables);
                return 0;
            case InstructionType::SUBTRACT:
                executeSubtract(variables);
                return 0;
            case InstructionType::SLEEP:
                return executeSleep();
            case InstructionType::FOR:
                executeFor(variables);
                return 0;
            default:
                std::cerr << "[ERROR] Unknown instruction: " << command << std::endl;
                return 0;
        }
    }

private:
    // Helper for reading variables or constants
    uint16_t getValue(const std::unordered_map<std::string, uint16_t> &vars,
                      const std::string &token) const {
        if (vars.find(token) != vars.end()) {
            return vars.at(token);
        }

        try {
            int val = std::stoi(token);
            if (val < 0) val = 0;
            if (val > 65535) val = 65535;
            return static_cast<uint16_t>(val);
        } catch (...) {
            return 0;
        }
    }

    // Print
    void executePrint(const std::unordered_map<std::string, uint16_t> &vars) const {
        if (args.empty()) {
            std::cout << "[PRINT] (no message)\n";
            return;
        }

        std::ostringstream msg;
        for (size_t i = 0; i < args.size(); ++i) {
            std::string token = args[i];
            if (token == "+") continue;

            if (vars.find(token) != vars.end()) {
                msg << vars.at(token);
            } else {
                if (token.size() >= 2 && token.front() == '"' && token.back() == '"')
                    token = token.substr(1, token.size() - 2);
                msg << token;
            }
        }

        std::cout << "[PRINT] " << msg.str() << std::endl;
    }

    // Declare
    void executeDeclare(std::unordered_map<std::string, uint16_t> &vars) const {
        if (args.size() < 2) {
            std::cerr << "[ERROR] DECLARE requires 2 arguments (var, value)\n";
            return;
        }

        std::string var = args[0];
        uint16_t value = getValue(vars, args[1]);
        vars[var] = value;

        std::cout << "[DECLARE] " << var << " = " << value << std::endl;
    }

    // Add
    void executeAdd(std::unordered_map<std::string, uint16_t> &vars) const {
        if (args.size() < 3) {
            std::cerr << "[ERROR] ADD requires 3 arguments (var1, var2/value, var3/value)\n";
            return;
        }

        std::string dest = args[0];
        uint16_t a = getValue(vars, args[1]);
        uint16_t b = getValue(vars, args[2]);
        uint32_t result = a + b;

        // use 32-bit to detect overflow
        if (result > 65535) result = 65535;
        vars[dest] = static_cast<uint16_t>(result);

        std::cout << "[ADD] " << dest << " = " << a << " + " << b << " = " << result << std::endl;
    }

    // Subtract
    void executeSubtract(std::unordered_map<std::string, uint16_t> &vars) const {
        if (args.size() < 3) {
            std::cerr << "[ERROR] SUBTRACT requires 3 arguments (var1, var2/value, var3/value)\n";
            return;
        }

        std::string dest = args[0];
        uint16_t a = getValue(vars, args[1]);
        uint16_t b = getValue(vars, args[2]);
        int32_t result = static_cast<int32_t>(a) - static_cast<int32_t>(b);

        if (result < 0) result = 0;
        vars[dest] = static_cast<uint16_t>(result);

        std::cout << "[SUBTRACT] " << dest << " = " << a << " - " << b << " = " << result << std::endl;
    }

    // Sleep
    int executeSleep() const {
        if (args.empty()) {
            std::cerr << "[ERROR] SLEEP requires 1 argument (ticks)\n";
            return 0;
        }

        try {
            int ticks = std::stoi(args[0]);
            if (ticks < 0) ticks = 0;
            if (ticks > 255) ticks = 255;

            std::cout << "[SLEEP] Sleeping for " << ticks << " ticks\n";
            return ticks; // return sleep duration for CPU to handle
        } catch (...) {
            std::cerr << "[ERROR] Invalid SLEEP argument: " << args[0] << std::endl;
            return 0;
        }
    }

    // For
    void executeFor(std::unordered_map<std::string, uint16_t> &vars) const {
        if (args.empty()) {
            std::cerr << "[ERROR] FOR requires repeat count\n";
            return;
        }

        uint16_t repeatCount = getValue(vars, args.back());
        std::cout << "[FOR] Repeating " << repeatCount << " times\n";

        for (uint16_t i = 0; i < repeatCount; ++i) {
            for (const auto &instr : subInstructions) {
                instr.execute(vars);
            }
        }
    }
};