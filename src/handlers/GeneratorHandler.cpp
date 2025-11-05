#pragma once

#include <memory>
#include <random>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include "../model/Process.cpp"
#include "../model/Config.cpp"

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
     */
    std::shared_ptr<Process> generateProcess(const Config &config)
    {
        // Generate process name
        std::string processName = "process_" + std::to_string(processCounter++);

        // Determine number of instructions
        std::uniform_int_distribution<int> distInstr(config.getMinInstructions(), config.getMaxInstructions());
        int numInstructions = distInstr(rng);

        // Create process
        auto process = std::make_shared<Process>(processName, numInstructions);

        // Track declared variables to avoid using undeclared ones
        std::unordered_set<std::string> declaredVars;
        std::uniform_int_distribution<int> valueDist(1, 100); // random int values
        std::uniform_int_distribution<int> instrTypeDist(0, 5); // random instruction type

        for (int i = 0; i < numInstructions; i++)
        {
            Instruction instr;

            int type = instrTypeDist(rng);
            switch (type)
            {
                case 0: // DECLARE
                {
                    std::string var = "v" + std::to_string(declaredVars.size() + 1);
                    int val = valueDist(rng);
                    instr = Instruction("DECLARE", {var, std::to_string(val)});
                    declaredVars.insert(var);
                    break;
                }

                case 1: // ADD
                {
                    if (declaredVars.size() < 1)
                        continue; // skip if no vars yet
                    std::vector<std::string> vars(declaredVars.begin(), declaredVars.end());
                    std::string dest = vars[rng() % vars.size()];
                    std::string op1 = vars[rng() % vars.size()];
                    std::string op2 = std::to_string(valueDist(rng));
                    instr = Instruction("ADD", {dest, op1, op2});
                    break;
                }

                case 2: // SUBTRACT
                {
                    if (declaredVars.size() < 1)
                        continue;
                    std::vector<std::string> vars(declaredVars.begin(), declaredVars.end());
                    std::string dest = vars[rng() % vars.size()];
                    std::string op1 = vars[rng() % vars.size()];
                    std::string op2 = std::to_string(valueDist(rng));
                    instr = Instruction("SUBTRACT", {dest, op1, op2});
                    break;
                }

                case 3: // PRINT
                {
                    if (!declaredVars.empty())
                    {
                        std::vector<std::string> vars(declaredVars.begin(), declaredVars.end());
                        std::string var = vars[rng() % vars.size()];
                        instr = Instruction("PRINT", {"\"Value of\"", "+", var});
                    }
                    else
                    {
                        instr = Instruction("PRINT", {"\"Hello World!\""});
                    }
                    break;
                }

                case 4: // SLEEP
                {
                    std::uniform_int_distribution<int> sleepDist(1, 50);
                    int ticks = sleepDist(rng);
                    instr = Instruction("SLEEP", {std::to_string(ticks)});
                    break;
                }

                case 5: // FOR
                {
                    // Create a mini block of subinstructions
                    std::uniform_int_distribution<int> forCountDist(2, 5);
                    int repeat = forCountDist(rng);

                    std::vector<Instruction> subInstrs;
                    int subCount = 2 + (rng() % 3); // 2–4 subinstructions

                    for (int j = 0; j < subCount; j++)
                    {
                        std::uniform_int_distribution<int> subTypeDist(0, 2);
                        int subType = subTypeDist(rng);

                        if (subType == 0)
                            subInstrs.push_back(Instruction("PRINT", {"\"Inside loop\"", "+", "\"iteration\""}));
                        else if (subType == 1 && !declaredVars.empty())
                        {
                            std::vector<std::string> vars(declaredVars.begin(), declaredVars.end());
                            std::string var = vars[rng() % vars.size()];
                            subInstrs.push_back(Instruction("ADD", {var, var, "1"}));
                        }
                        else
                            subInstrs.push_back(Instruction("SLEEP", {"5"}));
                    }

                    instr = Instruction("FOR", {std::to_string(repeat)});
                    instr.setSubInstructions(subInstrs);
                    break;
                }

                default:
                    instr = Instruction("PRINT", {"\"Unhandled instruction type\""});
                    break;
            }

            process->addInstruction(instr);
        }

        return process;
    }

    /**
     * Generates a batch of processes
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

    /**
     * Generates a process with a specific name
     */
    std::shared_ptr<Process> generateProcessWithName(const Config &config, const std::string &customName)
    {
        int numInstructions = std::uniform_int_distribution<int>(
            config.getMinInstructions(),
            config.getMaxInstructions()
        )(rng);

        auto process = std::make_shared<Process>(customName, numInstructions);

        // Use normal generation logic
        *process = *generateProcess(config);
        process->setState(ProcessState::READY);

        return process;
    }
};