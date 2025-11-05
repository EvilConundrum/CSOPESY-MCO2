#pragma once

#include <memory>
#include <random>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include "../model/Process.cpp"
#include "../model/Config.cpp"

/**
 * GeneratorHandler
 * ----------------
 * Generates processes and instructions according to emulator specs.
 *
 * SPEC REQUIREMENTS:
 * 1. Each process starts with variable "x" = 0.
 * 2. Instructions alternate strictly as:
 *      PRINT("Value from: " + x)
 *      ADD(x, x, [1–10])
 *      PRINT, ADD, PRINT, ADD, ...
 */
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
     * Generates a new process with alternating PRINT and ADD instructions.
     */
    std::shared_ptr<Process> generateProcess(const Config &config)
    {
        // 🔹 Generate process name (proc-01, proc-02, etc.)
        processCounter++;
        std::stringstream ss;
        ss << "proc-" << std::setfill('0') << std::setw(2) << processCounter;
        std::string processName = ss.str();

        // 🔹 Determine number of instructions
        std::uniform_int_distribution<int> distInstr(config.getMinInstructions(), config.getMaxInstructions());
        int numInstructions = distInstr(rng);

        // 🔹 Create the process
        auto process = std::make_shared<Process>(processName, numInstructions);

        // Each process already starts with variable x=0 (set inside Process constructor)

        // 🔹 Generate alternating instructions: PRINT / ADD
        std::uniform_int_distribution<int> addDist(1, 10);

        for (int i = 0; i < numInstructions; i++)
        {
            Instruction instr;

            if (i % 2 == 0)
            {
                // Even index — PRINT instruction
                instr = Instruction("PRINT", {"\"Value from: \"", "+", "x"});
            }
            else
            {
                // Odd index — ADD(x, x, random 1–10)
                int addVal = addDist(rng);
                instr = Instruction("ADD", {"x", "x", std::to_string(addVal)});
            }

            process->addInstruction(instr);
        }

        return process;
    }

    /**
     * Generates a batch of processes for testing or batch creation.
     */
    std::vector<std::shared_ptr<Process>> generateBatch(const Config &config, int batchSize)
    {
        std::vector<std::shared_ptr<Process>> processes;
        for (int i = 0; i < batchSize; i++)
        {
            processes.push_back(generateProcess(config));
        }
        return processes;
    }

    /**
     * Resets process counter back to 0.
     */
    void resetCounter()
    {
        processCounter = 0;
    }

    /**
     * Returns current number of generated processes.
     */
    int getProcessCount() const
    {
        return processCounter;
    }

    /**
     * Generates a process with a custom name, following same alternating rule.
     */
    std::shared_ptr<Process> generateProcessWithName(const Config &config, const std::string &customName)
    {
        int numInstructions = std::uniform_int_distribution<int>(
            config.getMinInstructions(),
            config.getMaxInstructions())(rng);

        auto process = std::make_shared<Process>(customName, numInstructions);

        // Alternate PRINT/ADD like standard generation
        std::uniform_int_distribution<int> addDist(1, 10);
        for (int i = 0; i < numInstructions; i++)
        {
            Instruction instr;
            if (i % 2 == 0)
                instr = Instruction("PRINT", {"\"Value from: \"", "+", "x"});
            else
                instr = Instruction("ADD", {"x", "x", std::to_string(addDist(rng))});

            process->addInstruction(instr);
        }

        process->setState(ProcessState::READY);
        return process;
    }
};