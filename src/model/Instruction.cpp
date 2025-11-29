#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <cstdint>
#include <functional>
#include "../model/memory/Memory.cpp"

enum class InstructionType
{
    PRINT,
    DECLARE,
    ADD,
    SUBTRACT,
    SLEEP, // aside from this, every other instruction needs to access memory
    FOR,
    READ,
    WRITE,
    UNKNOWN
};

class Instruction
{
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
        : command(command), args(arguments), lineNumber(0)
    {
        std::string cmd = command;
        for (auto &c : cmd)
            c = std::toupper(c);

        if (cmd == "PRINT")
            type = InstructionType::PRINT;
        else if (cmd == "DECLARE")
            type = InstructionType::DECLARE;
        else if (cmd == "ADD")
            type = InstructionType::ADD;
        else if (cmd == "SUBTRACT")
            type = InstructionType::SUBTRACT;
        else if (cmd == "SLEEP")
            type = InstructionType::SLEEP;
        else if (cmd == "FOR")
            type = InstructionType::FOR;
        else if (cmd == "READ")
            type = InstructionType::READ;
        else if (cmd == "WRITE")
            type = InstructionType::WRITE;
        else
            type = InstructionType::UNKNOWN;
    }

    void setSubInstructions(const std::vector<Instruction> &subs) { subInstructions = subs; }
    InstructionType getType() const { return type; }
    std::string getRaw() const
    {
        // return the raw instruction as a string
        std::ostringstream oss;
        oss << command;
        for (const auto &arg : args)
        {
            oss << " " << arg;
        }
        return oss.str();
    }
    std::string getCommand() const { return command; }
    std::vector<std::string> getArgs() const { return args; }
    int getLineNumber() const { return lineNumber; }
    bool isValid() const { return type != InstructionType::UNKNOWN; }

    /**
     * Executes the instruction.
     * Returns number of CPU ticks consumed:
     * - 0 for instant instructions
     * - N for SLEEP
     * - -1 for memory access violations (not sleep duration but indicates termination)
     */
    int execute(std::unordered_map<std::string, uint16_t> &variables,
                Memory *memory, uint64_t currentTick, std::vector<int> &allocatedPages, int allocatedMemory,
                std::function<void(const std::string &)> logCallback = nullptr,
                bool printToConsole = true) const
    {
        // TODO: Route READ/WRITE instruction types to dedicated handlers once Process exposes virtual memory APIs.
        switch (type)
        {
        case InstructionType::PRINT:
            executePrint(variables, memory, currentTick, allocatedPages, logCallback, printToConsole);
            return 0;
        case InstructionType::DECLARE:
            executeDeclare(variables, memory, currentTick, allocatedPages, allocatedMemory, logCallback, printToConsole);
            return 0;
        case InstructionType::ADD:
            executeAdd(variables, memory, currentTick, allocatedPages, logCallback, printToConsole);
            return 0;
        case InstructionType::SUBTRACT:
            executeSubtract(variables, memory, currentTick, allocatedPages, logCallback, printToConsole);
            return 0;
        case InstructionType::SLEEP:
            return executeSleep(logCallback, printToConsole);
        case InstructionType::FOR:
            executeFor(variables, memory, currentTick, allocatedPages, allocatedMemory, logCallback, printToConsole);
            return 0;
        case InstructionType::READ:
            return executeRead(variables, memory, currentTick, allocatedPages, allocatedMemory, logCallback, printToConsole);
        case InstructionType::WRITE:
            return executeWrite(variables, memory, currentTick, allocatedPages, allocatedMemory, logCallback, printToConsole);
        default:
        {
            std::string errMsg = "[ERROR] Unknown instruction: " + command;
            if (printToConsole)
                std::cerr << errMsg << std::endl;
            if (logCallback)
                logCallback(errMsg);
            return 0;
        }
        }
    }

private:
    /**
     * Given a variable name, returns its associated virtual address
     * returns UINT16_MAX if variable not found
     */
    VirtualAddress getAddress(const std::unordered_map<std::string, uint16_t> &vars, const std::string &token, int pageSize) const
    {
        auto it = vars.find(token);
        if (it != vars.end())
            return parseVirtualAddress(it->second, pageSize);
        return {UINT16_MAX, UINT16_MAX};
    }

    /**
     * Extracts the value of a token, which can be:
     * - a variable name (looked up in vars)
     * - a decimal integer string
     * - a hexadecimal string (starting with "0x" or "0X")
     *
     * returns the resolved uint16_t value, or 0 if parsing fails
     */
    // TODO: verify for correctness
    uint16_t getValue(Memory *memory, const std::unordered_map<std::string, uint16_t> &vars,
                      const std::string &token, std::vector<int> allocatedAddresses, uint64_t currentTick) const
    {
        // check if token is a variable
        auto it = vars.find(token);
        if (it != vars.end())
        {
            VirtualAddress addr = getAddress(vars, token, memory->getPageSize());
            LogicalAddress logicalAddr = mapLogicalToPhysical(memory, addr, allocatedAddresses);
            return memory->read(logicalAddr, currentTick);
        }

        bool isHex = token.rfind("0x", 0) == 0 || token.rfind("0X", 0) == 0;

        if (isHex)
        {
            // try parsing as hexadecimal integer
            try
            {
                uint16_t val = to_uint16(token);
                return val;
            }
            catch (...)
            {
                std::cout << "Failed to convert " << token << " to hex." << std::endl;
            }
        }
        else
        {

            // try parsing as decimal integer
            try
            {
                int val = std::stoi(token);
                // clamp to uint16_t range
                val = (val < 0) ? 0 : val;
                val = (val > 65535) ? 65535 : val;
                return static_cast<uint16_t>(val);
            }
            catch (...)
            {
                std::cout << "Failed to convert " << token << " to decimal." << std::endl;
            }
        }

        return 0;
    }

    void executePrint(const std::unordered_map<std::string, uint16_t> &vars,
                      Memory *memory, uint64_t currentTick, std::vector<int> &allocatedPages,
                      std::function<void(const std::string &)> logCallback,
                      bool printToConsole) const
    {
        std::ostringstream msg;
        for (const auto &token : args)
        {
            if (token == "+")
                continue;
            auto it = vars.find(token);
            if (it != vars.end())
            {
                msg << getValue(memory, vars, token, allocatedPages, currentTick);
            }
            else
            {
                std::string out = token;
                // check if token starts with substring \" and ends with \"
                if (out.rfind("\\\"", 0) == 0 && out.rfind("\\\"") == out.length() - 2)
                    out = out.substr(2, out.length() - 4); // remove quotes
                else if (out.front() == '"' && out.back() == '"')
                    out = out.substr(1, out.length() - 2); // remove quotes
                msg << out;
            }
        }
        std::string output = "[PRINT] " + msg.str();
        if (printToConsole)
            std::cout << output << std::endl;
        if (logCallback)
            logCallback(output);
    }

    // TODO: implement reading and writing variables from memory
    void executeDeclare(std::unordered_map<std::string, uint16_t> &vars,
                        Memory *memory, uint64_t currentTick, std::vector<int> &allocatedPages,
                        int allocatedMemory, std::function<void(const std::string &)> logCallback,
                        bool printToConsole) const
    {
        if (args.size() < 2)
        {
            std::string errMsg = "[ERROR] DECLARE requires 2 arguments (var, value)";
            if (printToConsole)
                std::cerr << errMsg << std::endl;
            if (logCallback)
                logCallback(errMsg);
            return;
        }

        // min(32, allocatedMemory / 2)
        int maxVars = (allocatedMemory >= 64) ? 32 : allocatedMemory / 2;

        if (vars.size() >= maxVars && vars.find(args[0]) == vars.end())
        {
            std::string errMsg = "[ERROR] Variable limit reached (" + std::to_string(maxVars) + " variables max)";
            if (printToConsole)
                std::cerr << errMsg << std::endl;
            if (logCallback)
                logCallback(errMsg);
            return;
        }

        std::string var = args[0];
        uint16_t value = getValue(memory, vars, args[1], allocatedPages, currentTick);
        vars[var] = vars.size() * 2; // assign address to first page or bytes 0 to 63

        // write to memory
        LogicalAddress addr = getLogicalAddress(memory, vars[var], allocatedPages); // always allocate to first page, offset 0
        memory->write(addr, value, currentTick);

        std::string output = "[DECLARE] " + var + " = " + std::to_string(value);
        if (printToConsole)
            std::cout << output << std::endl;
        if (logCallback)
            logCallback(output);
    }

    void executeAdd(std::unordered_map<std::string, uint16_t> &vars,
                    Memory *memory, uint64_t currentTick, std::vector<int> &allocatedPages,
                    std::function<void(const std::string &)> logCallback,
                    bool printToConsole) const
    {
        if (args.size() < 3)
        {
            std::string errMsg = "[ERROR] ADD requires 3 arguments";
            if (printToConsole)
                std::cerr << errMsg << std::endl;
            if (logCallback)
                logCallback(errMsg);
            return;
        }
        std::string dest = args[0];
        uint16_t a = getValue(memory, vars, args[1], allocatedPages, currentTick);
        uint16_t b = getValue(memory, vars, args[2], allocatedPages, currentTick);
        uint32_t result = a + b;
        if (result > 65535)
            result = 65535;

        LogicalAddress addr = getLogicalAddress(memory, vars[dest], allocatedPages); // always allocate to first page, offset 0
        memory->write(addr, static_cast<uint16_t>(result), currentTick);

        std::string output = "[ADD] " + dest + " = " + std::to_string(a) + " + " + std::to_string(b) + " = " + std::to_string(result);
        if (printToConsole)
            std::cout << output << std::endl;
        if (logCallback)
            logCallback(output);
    }

    // TODO: implement reading and writing variables from memory
    void executeSubtract(std::unordered_map<std::string, uint16_t> &vars,
                         Memory *memory, uint64_t currentTick, std::vector<int> &allocatedPages,
                         std::function<void(const std::string &)> logCallback,
                         bool printToConsole) const
    {
        if (args.size() < 3)
        {
            std::string errMsg = "[ERROR] SUBTRACT requires 3 arguments";
            if (printToConsole)
                std::cerr << errMsg << std::endl;
            if (logCallback)
                logCallback(errMsg);
            return;
        }
        std::string dest = args[0];
        uint16_t a = getValue(memory, vars, args[1], allocatedPages, currentTick);
        uint16_t b = getValue(memory, vars, args[2], allocatedPages, currentTick);
        int32_t result = static_cast<int32_t>(a) - static_cast<int32_t>(b);
        if (result < 0)
            result = 0;
        vars[dest] = static_cast<uint16_t>(result);
        std::string output = "[SUBTRACT] " + dest + " = " + std::to_string(a) + " - " + std::to_string(b) + " = " + std::to_string(result);
        if (printToConsole)
            std::cout << output << std::endl;
        if (logCallback)
            logCallback(output);
    }

    // no TODO, this function is unmodified from MO1
    int executeSleep(std::function<void(const std::string &)> logCallback,
                     bool printToConsole) const
    {
        if (args.empty())
        {
            std::string errMsg = "[ERROR] SLEEP requires 1 argument (ticks)";
            if (printToConsole)
                std::cerr << errMsg << std::endl;
            if (logCallback)
                logCallback(errMsg);
            return 0;
        }
        int ticks = 0;
        try
        {
            ticks = std::stoi(args[0]);
        }
        catch (...)
        {
            std::string errMsg = "[ERROR] Invalid SLEEP argument: " + args[0];
            if (printToConsole)
                std::cerr << errMsg << std::endl;
            if (logCallback)
                logCallback(errMsg);
            return 0;
        }

        if (ticks < 0)
            ticks = 0;
        if (ticks > 255)
            ticks = 255;

        std::string output = "[SLEEP] Sleeping for " + std::to_string(ticks) + " ticks";
        if (printToConsole)
            std::cout << output << std::endl;
        if (logCallback)
            logCallback(output);

        return ticks;
    }

    int executeFor(std::unordered_map<std::string, uint16_t> &vars,
                   Memory *memory, uint64_t currentTick, std::vector<int> &allocatedPages,
                   int allocatedMemory, std::function<void(const std::string &)> logCallback,
                   bool printToConsole) const
    {
        if (args.empty())
        {
            std::string errMsg = "[ERROR] FOR requires repeat count";
            if (printToConsole)
                std::cerr << errMsg << std::endl;
            if (logCallback)
                logCallback(errMsg);
            return 0;
        }

        // Use the last argument as repeat count
        uint16_t repeatCount = getValue(memory, vars, args.back(), allocatedPages, currentTick);

        std::string header = "[FOR] Repeating " + std::to_string(repeatCount) + " times";
        if (printToConsole)
            std::cout << header << std::endl;
        if (logCallback)
            logCallback(header);

        // Execute subinstructions repeatCount times
        for (uint16_t i = 0; i < repeatCount; ++i)
            for (const auto &instr : subInstructions)
                instr.execute(vars, memory, currentTick, allocatedPages, allocatedMemory, logCallback, printToConsole);

        // FOR itself is considered instant (0 ticks) for unit tests
        return 0;
    }

    // TODO: verify function for correctness
    int executeRead(std::unordered_map<std::string, uint16_t> &vars,
                    Memory *memory, uint64_t currentTick, std::vector<int> &allocatedPages,
                    int allocatedMemory, std::function<void(const std::string &)> logCallback,
                    bool printToConsole) const
    {
        // Validate args[0] as destination variable name and args[1] as hex address.
        if (args.size() < 2)
        {
            std::string errMsg = "[ERROR] READ requires 2 arguments (var, address)";
            if (printToConsole)
                std::cerr << errMsg << std::endl;
            if (logCallback)
                logCallback(errMsg);
            return 0;
        }

        VirtualAddress dest = getAddress(vars, args[0], memory->getPageSize());
        uint16_t src = getValue(memory, vars, args[1], allocatedPages, currentTick);

        LogicalAddress destAddr = mapLogicalToPhysical(memory, dest, allocatedPages);
        LogicalAddress srcAddr = getLogicalAddress(memory, src, allocatedPages, allocatedMemory);

        // no need to check destAddr validity as it's from vars
        if (srcAddr.pageNumber == -1 || srcAddr.offset == -1)
        {
            std::string header = "[READ] Access violation at address " + args[1];
            if (printToConsole)
                std::cerr << header << std::endl;
            if (logCallback)
                logCallback(header);
            return -1;
        }

        uint16_t value = memory->read(srcAddr, currentTick);
        // write value to destination variable
        memory->write(destAddr, value, currentTick);

        // log read
        std::string header = "[READ] Read value " + std::to_string(value) + " from address " + args[1] + " into variable " + args[0];
        if (printToConsole)
            std::cout << header << std::endl;
        if (logCallback)
            logCallback(header);

        // 0 = successful read
        return 0;
    }

    // TODO: verify function for correctness
    int executeWrite(std::unordered_map<std::string, uint16_t> &vars,
                     Memory *memory, uint64_t currentTick, std::vector<int> &allocatedPages,
                     int allocatedMemory, std::function<void(const std::string &)> logCallback,
                     bool printToConsole) const
    {

        // Sample: WRITE 0x500 varC

        // TODO: Validate args[0] as hex address and args[1] as source (literal or variable name).
        if (args.size() < 2)
        {
            std::string errMsg = "[ERROR] WRITE requires 2 arguments (address, value)";
            if (printToConsole)
                std::cerr << errMsg << std::endl;
            if (logCallback)
                logCallback(errMsg);
            return 0;
        }

        uint16_t dest = getValue(memory, vars, args[0], allocatedPages, currentTick),
                 src = getValue(memory, vars, args[1], allocatedPages, currentTick);

        LogicalAddress destAddr = getLogicalAddress(memory, dest, allocatedPages, allocatedMemory);

        // check for access violation
        // returns -1 to indicate access violation
        if (destAddr.pageNumber == -1 || destAddr.offset == -1)
        {
            std::string header = "[WRITE] Access violation at address " + args[0];
            if (printToConsole)
                std::cerr << header << std::endl;
            if (logCallback)
                logCallback(header);
            return -1;
        }

        // write to memory
        memory->write(destAddr, src, currentTick);

        // log write
        std::string header = "[WRITE] Writing value " + std::to_string(src) + " to address " + args[0];
        if (printToConsole)
            std::cout << header << std::endl;
        if (logCallback)
            logCallback(header);

        // 0 = successful write
        return 0;
    }

    /**
     * translates the logical address to physical page number and offset using allocatedPages
     * returns converted LogicalAddress if found, returns {-1, -1} if access violation
     */
    LogicalAddress getLogicalAddress(Memory *memory, uint16_t address, std::vector<int> &allocatedPages, int allocatedMemory = -1) const
    {
        // if allocated memory is provided, use it to determine access violation
        if (allocatedMemory != -1)
            if (address < 0 || address >= static_cast<uint16_t>(allocatedMemory))
                return {-1, -1};

        VirtualAddress virtualAddress = parseVirtualAddress(address, memory->getPageSize());
        LogicalAddress logicalAddress = mapLogicalToPhysical(memory, virtualAddress, allocatedPages);
        return logicalAddress;
    }

    /**
     * translates the logical address to physical page number and offset using allocatedPages
     * returns converted LogicalAddress if found, returns {-1, -1} if access violation
     */
    LogicalAddress mapLogicalToPhysical(Memory *memory, VirtualAddress virtualAddress, std::vector<int> &allocatedPages) const
    {
        int pageNumber = virtualAddress.pageNumber;
        // if page number is outside of the allocated pages, access violation
        int numPages = static_cast<int>(allocatedPages.size());
        if (pageNumber < 0 || pageNumber >= numPages)
        {
            return {-1, -1};
        }

        LogicalAddress logicalAddress = {allocatedPages[pageNumber], virtualAddress.offset};

        // std::cout << "Translating virtual page number " << virtualAddress.str()
        //           << " to physical frame number " << logicalAddress.str() << std::endl;

        // if valid, return the translated physical address
        return logicalAddress;
    }
};