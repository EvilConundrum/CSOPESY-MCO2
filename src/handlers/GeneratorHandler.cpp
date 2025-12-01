#pragma once

#include <memory>
#include <random>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <unordered_set>
#include <algorithm>
#include "../model/Process.cpp"
#include "../model/Config.cpp"
#include "../model/memory/Address.cpp"

class GeneratorHandler
{
    int processCounter;
    int latestViolation;
    std::mt19937 rng;

public:
    GeneratorHandler()
    {
        processCounter = 0;
        latestViolation = -1;
        rng.seed(std::chrono::system_clock::now().time_since_epoch().count());
    }

    /**
     * Generates a new process with random instructions based on config
     * Process naming follows spec: p01, p02, ..., p1240
     */
    std::shared_ptr<Process> generateProcess(const Config &config, std::shared_ptr<Memory> memory)
    {
        // Generate process name with proper formatting (p01, p02, etc.)
        processCounter++;
        std::stringstream ss;
        ss << "p" << std::setfill('0') << std::setw(2) << processCounter;
        std::string processName = ss.str();

        std::uniform_int_distribution<int> distMem(config.getMinMemPerProc(), config.getMaxMemPerProc());
        int memoryRequired = distMem(rng);

        return generateProcessWithName(config, processName, memory.get(), memoryRequired);
    }

    /**
     * Generates a process with a specific name
     */
    std::shared_ptr<Process> generateProcessWithName(const Config &config, const std::string &customName, Memory *memory, int memoryRequired)
    {
        int numInstructions = std::uniform_int_distribution<int>(
            config.getMinInstructions(),
            config.getMaxInstructions())(rng);

        // Create process with custom name
        auto process = std::make_shared<Process>(customName, numInstructions, memoryRequired);

        // Allocate memory pages
        process->addPageNumbers(memory->makePages(memoryRequired));

        // Track declared variables to avoid using undeclared ones
        std::vector<std::string> declaredVars;

        int i = 0;
        while (i < numInstructions)
        {
            Instruction instr = generateRandomInstruction(declaredVars, process->getNumPages(), config.getMemPerFrame(), memoryRequired);

            if (instr.getType() == InstructionType::UNKNOWN)
                continue;

            process->addInstruction(instr);
            i++;
        }

        process->setState(ProcessState::READY);
        return process;
    }

    /**
     * Resets the process counter
     */
    void resetCounter()
    {
        processCounter = 0;
    }

    /**
     * Gets the current process count
     */
    int getProcessCount() const
    {
        return processCounter;
    }

private:
    /**
     * Helper to ensure that variable access only reads
     * Each variable is 2 bytes
     * For processes with 64 or more bytes allocated, we can have up to 32 variables
     * For processes with less than 64 bytes allocated, the max number of variables is:
     *   requiredMemory // 2 or the current number of declared variables, whichever is less
     * This is not called by the create declare as in execution,
     * extra variables declared after that are ignored when interpreted
     * This is for ensuring the other instructions are making valid variable accesses
     */
    int maxSymbolTable(int requiredMemory, int currentNumVars)
    {
        int maxVarsForProcess = requiredMemory / 2;
        int limit;

        if (requiredMemory >= 64)
            limit = 32;
        else
            limit = std::min(maxVarsForProcess, currentNumVars);

        // Ensure we never return more than actually declared
        return std::min(limit, currentNumVars);
    }

    Instruction generateRandomInstruction(std::vector<std::string> &declaredVars, int numAllocatedPages, int pageSize, int requiredMemory)
    {
        std::uniform_int_distribution<int> valueDist(0, 10);    // random int values
        std::uniform_int_distribution<int> instrTypeDist(0, 7); // random instruction

        Instruction instr;

        int type = instrTypeDist(rng);
        int numVars = maxSymbolTable(requiredMemory, static_cast<int>(declaredVars.size()));
        switch (type)
        {
        case 0: // DECLARE
            instr = createDeclareInstruction(valueDist, declaredVars);
            break;
        case 1: // ADD
            instr = createAddInstruction(valueDist, declaredVars, numVars);
            break;
        case 2: // SUBTRACT
            instr = createSubtractInstruction(valueDist, declaredVars, numVars);
            break;
        case 3: // PRINT
            instr = createPrintInstruction(declaredVars, numVars);
            break;
        case 4: // SLEEP
            instr = createSleepInstruction(valueDist);
            break;
        case 5: // FOR
            instr = createForInstruction(declaredVars, numVars);
            break;

        case 6: // READ
            instr = createReadInstruction(declaredVars, requiredMemory, numVars);
            break;

        case 7: // WRITE
        {
            instr = createWriteInstruction(declaredVars, requiredMemory, numVars);
            break;
        }

        default:
            instr = Instruction("PRINT", {"\"Unhandled instruction type\""});
            break;
        }
        return instr;
    }

    Instruction createDeclareInstruction(std::uniform_int_distribution<int> &valueDist, std::vector<std::string> &declaredVars)
    {
        std::string var = "v" + std::to_string(declaredVars.size() + 1);
        int val = valueDist(rng);
        Instruction instr = Instruction("DECLARE", {var, std::to_string(val)});
        declaredVars.push_back(var);
        return instr;
    }

    Instruction createAddInstruction(std::uniform_int_distribution<int> &valueDist, const std::vector<std::string> &declaredVars, int numVars)
    {
        if (declaredVars.size() < 1)
            return Instruction(); // Return unknown instruction to skip
        std::string dest = declaredVars.at(rng() % numVars);
        std::string op1 = declaredVars.at(rng() % numVars);
        std::string op2 = std::to_string(valueDist(rng));
        return Instruction("ADD", {dest, op1, op2});
    }

    Instruction createSubtractInstruction(std::uniform_int_distribution<int> &valueDist, const std::vector<std::string> &declaredVars, int numVars)
    {
        if (declaredVars.size() < 1)
            return Instruction(); // Return unknown instruction to skip
        std::string dest = declaredVars.at(rng() % numVars);
        std::string op1 = declaredVars.at(rng() % numVars);
        std::string op2 = std::to_string(valueDist(rng));
        return Instruction("SUBTRACT", {dest, op1, op2});
    }

    Instruction createPrintInstruction(const std::vector<std::string> &declaredVars, int numVars)
    {
        if (declaredVars.size() < 1)
            return Instruction("PRINT", {"\"Hello World\""});

        std::string var = declaredVars.at(rng() % numVars);
        return Instruction("PRINT", {"\"Value of\"", "+", var});
    }

    Instruction createSleepInstruction(std::uniform_int_distribution<int> &valueDist)
    {
        std::uniform_int_distribution<int> sleepDist(1, 50);
        int ticks = sleepDist(rng);
        return Instruction("SLEEP", {std::to_string(ticks)});
    }

    Instruction createForInstruction(const std::vector<std::string> &declaredVars, int numVars)
    {
        std::uniform_int_distribution<int> forCountDist(2, 5);
        int repeat = forCountDist(rng);

        std::vector<Instruction> subInstrs;
        int subCount = 2 + (rng() % 3);

        for (int j = 0; j < subCount; j++)
        {
            std::uniform_int_distribution<int> subTypeDist(0, 2);
            int subType = subTypeDist(rng);

            if (subType == 0)
                subInstrs.push_back(Instruction("PRINT", {"\"Inside loop\"", "+", "\"iteration\""}));
            else if (subType == 1 && !declaredVars.empty())
            {
                std::string var = declaredVars.at(rng() % numVars);
                subInstrs.push_back(Instruction("ADD", {var, var, "1"}));
            }
            else
                subInstrs.push_back(Instruction("SLEEP", {"5"}));
        }

        Instruction instr = Instruction("FOR", {std::to_string(repeat)});
        instr.setSubInstructions(subInstrs);

        return instr;
    }

    Instruction createReadInstruction(const std::vector<std::string> &declaredVars, int requiredMemory, int numVars)
    {
        // Randomly decide to cause a violation (1 in 2^15 chance)
        std::uniform_int_distribution<uint64_t> violationDist(1, 32768);
        // tldr: we only cause a violation if both rng is met and it is either the first violation or 1000 processes since the first violation
        // we want this to be relatively rare
        bool causeViolation = (violationDist(rng) == processCounter && (latestViolation != -1 || latestViolation > (processCounter - 1000)));

        if (declaredVars.size() < 1)
            return Instruction(); // Return unknown instruction to skip

        // select from a valid address
        std::uniform_int_distribution<int> pageDist(0, requiredMemory - 1);
        uint16_t address = pageDist(rng) & 0xFFFE; // floor to even

        // add a random number to the last page index to go out of bounds
        if (causeViolation)
            address = requiredMemory + (rng() % (1 << 5)) * 2;
        std::string var = declaredVars.at(rng() % numVars);
        return Instruction("READ", {var, to_hex(address)});
    }

    Instruction createWriteInstruction(const std::vector<std::string> &declaredVars, int requiredMemory, int numVars)
    {
        if (declaredVars.size() < 1 || requiredMemory <= 64)
            return Instruction(); // Return unknown instruction to skip

        std::string var = declaredVars.at(rng() % numVars);
        std::uniform_int_distribution<int> valueDist(64, requiredMemory - 1); // first 64 addresses are reserved for variables
        uint16_t val = valueDist(rng) & 0xFFFE;                               // make it even
        return Instruction("WRITE", {to_hex(val), var});
    }
};