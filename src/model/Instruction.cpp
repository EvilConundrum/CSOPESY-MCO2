#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <cstdint>
#include <functional>

enum class InstructionType {
    PRINT,
    DECLARE,
    ADD,
    SUBTRACT,
    SLEEP,
    FOR,
    // TODO: Add READ and WRITE instruction types to support simulated memory ops via Process.
    UNKNOWN
};

class Instruction {
    InstructionType type;
    std::string command;
    std::vector<std::string> args;
    int lineNumber;
    std::vector<Instruction> subInstructions;

public:
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
        // TODO: Parse READ/WRITE tokens and map to new InstructionType entries once Process exposes memory ops.
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
     * Executes the instruction.
     * Returns number of CPU ticks consumed:
     * - 0 for instant instructions
     * - N for SLEEP
     */
    int execute(std::unordered_map<std::string, uint16_t> &variables,
                std::function<void(const std::string&)> logCallback = nullptr,
                bool printToConsole = true) const {
        // TODO: Route READ/WRITE instruction types to dedicated handlers once Process exposes virtual memory APIs.
        switch (type) {
            case InstructionType::PRINT:
                executePrint(variables, logCallback, printToConsole);
                return 0;
            case InstructionType::DECLARE:
                executeDeclare(variables, logCallback, printToConsole);
                return 0;
            case InstructionType::ADD:
                executeAdd(variables, logCallback, printToConsole);
                return 0;
            case InstructionType::SUBTRACT:
                executeSubtract(variables, logCallback, printToConsole);
                return 0;
            case InstructionType::SLEEP:
                return executeSleep(logCallback, printToConsole);
            case InstructionType::FOR:
                executeFor(variables, logCallback, printToConsole);
                return 0;
            default:
                {
                    std::string errMsg = "[ERROR] Unknown instruction: " + command;
                    if (printToConsole) std::cerr << errMsg << std::endl;
                    if (logCallback) logCallback(errMsg);
                    return 0;
                }
        }
    }

private:
    uint16_t getValue(const std::unordered_map<std::string, uint16_t> &vars,
                      const std::string &token) const {
        auto it = vars.find(token);
        if (it != vars.end()) return it->second;
        try {
            int val = std::stoi(token);
            if (val < 0) val = 0;
            if (val > 65535) val = 65535;
            return static_cast<uint16_t>(val);
        } catch (...) {
            return 0;
        }
    }

    void executePrint(const std::unordered_map<std::string, uint16_t> &vars,
                      std::function<void(const std::string&)> logCallback,
                      bool printToConsole) const {
        std::ostringstream msg;
        for (const auto &token : args) {
            if (token == "+") continue;
            auto it = vars.find(token);
            if (it != vars.end()) {
                msg << it->second;
            } else {
                std::string out = token;
                if (out.size() >= 2 && out.front() == '"' && out.back() == '"')
                    out = out.substr(1, out.size() - 2);
                msg << out;
            }
        }
        std::string output = "[PRINT] " + msg.str();
        if (printToConsole) std::cout << output << std::endl;
        if (logCallback) logCallback(output);
    }

    void executeDeclare(std::unordered_map<std::string, uint16_t> &vars,
                        std::function<void(const std::string&)> logCallback,
                        bool printToConsole) const {
        if (args.size() < 2) {
            std::string errMsg = "[ERROR] DECLARE requires 2 arguments (var, value)";
            if (printToConsole) std::cerr << errMsg << std::endl;
            if (logCallback) logCallback(errMsg);
            return;
        }
        // TODO: Enforce 32-variable / 64-byte symbol table cap before adding new entries.
        std::string var = args[0];
        uint16_t value = getValue(vars, args[1]);
        vars[var] = value;
        std::string output = "[DECLARE] " + var + " = " + std::to_string(value);
        if (printToConsole) std::cout << output << std::endl;
        if (logCallback) logCallback(output);
    }

    void executeAdd(std::unordered_map<std::string, uint16_t> &vars,
                    std::function<void(const std::string&)> logCallback,
                    bool printToConsole) const {
        if (args.size() < 3) {
            std::string errMsg = "[ERROR] ADD requires 3 arguments";
            if (printToConsole) std::cerr << errMsg << std::endl;
            if (logCallback) logCallback(errMsg);
            return;
        }
        std::string dest = args[0];
        uint16_t a = getValue(vars, args[1]);
        uint16_t b = getValue(vars, args[2]);
        uint32_t result = a + b;
        if (result > 65535) result = 65535;
        vars[dest] = static_cast<uint16_t>(result);
        std::string output = "[ADD] " + dest + " = " + std::to_string(a) + " + " + std::to_string(b) + " = " + std::to_string(result);
        if (printToConsole) std::cout << output << std::endl;
        if (logCallback) logCallback(output);
    }

    void executeSubtract(std::unordered_map<std::string, uint16_t> &vars,
                         std::function<void(const std::string&)> logCallback,
                         bool printToConsole) const {
        if (args.size() < 3) {
            std::string errMsg = "[ERROR] SUBTRACT requires 3 arguments";
            if (printToConsole) std::cerr << errMsg << std::endl;
            if (logCallback) logCallback(errMsg);
            return;
        }
        std::string dest = args[0];
        uint16_t a = getValue(vars, args[1]);
        uint16_t b = getValue(vars, args[2]);
        int32_t result = static_cast<int32_t>(a) - static_cast<int32_t>(b);
        if (result < 0) result = 0;
        vars[dest] = static_cast<uint16_t>(result);
        std::string output = "[SUBTRACT] " + dest + " = " + std::to_string(a) + " - " + std::to_string(b) + " = " + std::to_string(result);
        if (printToConsole) std::cout << output << std::endl;
        if (logCallback) logCallback(output);
    }

    int executeSleep(std::function<void(const std::string&)> logCallback,
                     bool printToConsole) const {
        if (args.empty()) {
            std::string errMsg = "[ERROR] SLEEP requires 1 argument (ticks)";
            if (printToConsole) std::cerr << errMsg << std::endl;
            if (logCallback) logCallback(errMsg);
            return 0;
        }
        int ticks = 0;
        try {
            ticks = std::stoi(args[0]);
        } catch (...) {
            std::string errMsg = "[ERROR] Invalid SLEEP argument: " + args[0];
            if (printToConsole) std::cerr << errMsg << std::endl;
            if (logCallback) logCallback(errMsg);
            return 0;
        }

        if (ticks < 0) ticks = 0;
        if (ticks > 255) ticks = 255;

        std::string output = "[SLEEP] Sleeping for " + std::to_string(ticks) + " ticks";
        if (printToConsole) std::cout << output << std::endl;
        if (logCallback) logCallback(output);

        return ticks;
    }

    int executeFor(std::unordered_map<std::string, uint16_t> &vars,
                   std::function<void(const std::string&)> logCallback,
                   bool printToConsole) const {
        if (args.empty()) {
            std::string errMsg = "[ERROR] FOR requires repeat count";
            if (printToConsole) std::cerr << errMsg << std::endl;
            if (logCallback) logCallback(errMsg);
            return 0;
        }

        // Use the last argument as repeat count
        uint16_t repeatCount = getValue(vars, args.back());

        std::string header = "[FOR] Repeating " + std::to_string(repeatCount) + " times";
        if (printToConsole) std::cout << header << std::endl;
        if (logCallback) logCallback(header);

        // Execute subinstructions repeatCount times
        for (uint16_t i = 0; i < repeatCount; ++i) {
            for (const auto &instr : subInstructions) {
                instr.execute(vars, logCallback, printToConsole);
            }
        }

        // FOR itself is considered instant (0 ticks) for unit tests
        return 0;
    }

    void executeRead(std::unordered_map<std::string, uint16_t> &vars,
                     std::function<void(const std::string&)> logCallback,
                     bool printToConsole) const {
        // TODO: Validate args[0] as destination variable and args[1] as hex address string.
        // TODO: Enforce symbol-table capacity before creating/updating dest.
        // TODO: Parse address, call into Process-managed virtual memory (e.g., Process::readUInt16) and store result.
        // TODO: Log success, and propagate Process memory faults so the offending process can terminate.
        (void)vars;
        (void)logCallback;
        (void)printToConsole;
    }

    void executeWrite(std::unordered_map<std::string, uint16_t> &vars,
                      std::function<void(const std::string&)> logCallback,
                      bool printToConsole) const {
        // TODO: Validate args[0] as hex address and args[1] as source (literal or variable name).
        // TODO: Clamp the resolved uint16_t value before writing.
        // TODO: Invoke Process::writeUInt16 (or equivalent) and log the outcome.
        // TODO: Surface access violations so the owning process can be shut down.
        (void)vars;
        (void)logCallback;
        (void)printToConsole;
    }

};
