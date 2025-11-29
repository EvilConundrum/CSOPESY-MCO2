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
    std::mt19937 rng;

public:
    GeneratorHandler()
    {
        processCounter = 0;
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

        // Determine number of instructions
        std::uniform_int_distribution<int> distInstr(config.getMinInstructions(), config.getMaxInstructions());
        int numInstructions = distInstr(rng);

        // determine amount of memory to be allocated to this process

        std::uniform_int_distribution<int> distMem(config.getMinMemPerProc(), config.getMaxMemPerProc());
        int memoryRequired = distMem(rng);

        // Create process
        auto process = std::make_shared<Process>(processName, numInstructions, memoryRequired);

        // Allocate memory pages
        process->addPageNumbers(memory.get()->makePages(memoryRequired));

        // Track declared variables to avoid using undeclared ones
        std::vector<std::string> declaredVars;

        int i = 0;
        while (i < numInstructions)
        {
            Instruction instr = generateRandomInstruction(declaredVars, process->getNumPages(), config.getMemPerFrame());
            if (instr.getType() == InstructionType::UNKNOWN)
                continue;

            process->addInstruction(instr);
            ++i;
        }

        return process;
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
            Instruction instr = generateRandomInstruction(declaredVars, process->getNumPages(), config.getMemPerFrame());

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
     * the first 32 variables as defined by the limit of the symbol table
     * Every other variable declared after that is ignored when interpreted
     */
    int maxSymbolTable(int current)
    {
        return (current > 32) ? 32 : current;
    }

    Instruction generateRandomInstruction(std::vector<std::string> &declaredVars, int numAllocatedPages, int pageSize)
    {
        std::uniform_int_distribution<int> valueDist(0, 10);    // random int values
        std::uniform_int_distribution<int> instrTypeDist(0, 7); // random instruction

        Instruction instr;

        int type = instrTypeDist(rng);
        switch (type)
        {
        case 0: // DECLARE
            instr = createDeclareInstruction(valueDist, declaredVars);
            break;
        case 1: // ADD
            instr = createAddInstruction(valueDist, declaredVars);
            break;
        case 2: // SUBTRACT
            instr = createSubtractInstruction(valueDist, declaredVars);
            break;
        case 3: // PRINT
            instr = createPrintInstruction(declaredVars);
            break;
        case 4: // SLEEP
            instr = createSleepInstruction(valueDist);
            break;
        case 5: // FOR
            instr = createForInstruction(declaredVars);
            break;

        case 6: // READ
            instr = createReadInstruction(declaredVars, numAllocatedPages, pageSize);
            break;

        case 7: // WRITE
        {
            instr = createWriteInstruction(declaredVars, numAllocatedPages, pageSize);
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

    Instruction createAddInstruction(std::uniform_int_distribution<int> &valueDist, const std::vector<std::string> &declaredVars)
    {
        if (declaredVars.size() < 1)
            return Instruction(); // Return unknown instruction to skip
        std::string dest = declaredVars[rng() % maxSymbolTable(declaredVars.size())];
        std::string op1 = declaredVars[rng() % maxSymbolTable(declaredVars.size())];
        std::string op2 = std::to_string(valueDist(rng));
        return Instruction("ADD", {dest, op1, op2});
    }

    Instruction createSubtractInstruction(std::uniform_int_distribution<int> &valueDist, const std::vector<std::string> &declaredVars)
    {
        if (declaredVars.size() < 1)
            return Instruction(); // Return unknown instruction to skip
        std::vector<std::string> vars(declaredVars.begin(), declaredVars.end());
        std::string dest = vars[rng() % maxSymbolTable(vars.size())];
        std::string op1 = vars[rng() % maxSymbolTable(vars.size())];
        std::string op2 = std::to_string(valueDist(rng));
        return Instruction("SUBTRACT", {dest, op1, op2});
    }

    Instruction createPrintInstruction(const std::vector<std::string> &declaredVars)
    {
        if (declaredVars.size() < 1)
            return Instruction("PRINT", {"\"Hello World\""});

        std::vector<std::string> vars(declaredVars.begin(), declaredVars.end());
        std::string var = vars[rng() % maxSymbolTable(vars.size())];
        return Instruction("PRINT", {"\"Value of\"", "+", var});
    }

    Instruction createSleepInstruction(std::uniform_int_distribution<int> &valueDist)
    {
        std::uniform_int_distribution<int> sleepDist(1, 50);
        int ticks = sleepDist(rng);
        return Instruction("SLEEP", {std::to_string(ticks)});
    }

    Instruction createForInstruction(const std::vector<std::string> &declaredVars)
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
                std::vector<std::string> vars(declaredVars.begin(), declaredVars.end());
                std::string var = vars[rng() % maxSymbolTable(vars.size())];
                subInstrs.push_back(Instruction("ADD", {var, var, "1"}));
            }
            else
                subInstrs.push_back(Instruction("SLEEP", {"5"}));
        }

        Instruction instr = Instruction("FOR", {std::to_string(repeat)});
        instr.setSubInstructions(subInstrs);

        return instr;
    }

    Instruction createReadInstruction(const std::vector<std::string> &declaredVars, int numPages, int pageSize)
    {
        // Randomly decide to cause a violation (1 in 32768 chance)
        std::uniform_int_distribution<int> violationDist(1, 32768);
        bool causeViolation = (violationDist(rng) == 67);

        if (declaredVars.size() < 1)
            return Instruction(); // Return unknown instruction to skip

        // select from a valid address
        std::uniform_int_distribution<int> pageDist(0, numPages * pageSize - 1);
        uint16_t address = pageDist(rng) & 0xFFFE; // floor to even

        // add a random number to the last page index to go out of bounds
        if (causeViolation)
            address = numPages * pageSize + (rng() % (1 << 5)) * 2;
        std::vector<std::string> vars(declaredVars.begin(), declaredVars.end());
        std::string var = vars[rng() % maxSymbolTable(vars.size())];
        return Instruction("READ", {var, to_hex(address)});
    }

    Instruction createWriteInstruction(const std::vector<std::string> &declaredVars, int numPages, int pageSize)
    {
        if (declaredVars.size() < 1)
            return Instruction(); // Return unknown instruction to skip

        std::string var = declaredVars[rng() % maxSymbolTable(declaredVars.size())];
        std::uniform_int_distribution<int> valueDist(64, numPages * pageSize - 1); // first 64 addresses are reserved for variables
        uint16_t val = valueDist(rng) & 0xFFFE;                                    // make it even
        return Instruction("WRITE", {to_hex(val), var});
    }
};