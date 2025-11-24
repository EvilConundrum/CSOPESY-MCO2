#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <atomic>
#include "../model/Config.cpp"

enum Commands
{
    EMPTY = -3,
    UNKNOWN,
    UNINITIALIZED,
    INITIALIZE,
    EXIT,
    SCREEN,
    SCHEDULER_START,
    SCHEDULER_STOP,
    PROCESS_SMI,
    REPORT_UTIL,
};

class CommandHandler
{
public:
    CommandHandler() {}

    void parseCommand(std::vector<std::string> args, std::atomic<bool> &isRunning, std::atomic<bool> &isInitialized, std::atomic<Commands> &opcode, std::atomic<int> &screenMode)
    {
        if (args.empty())
        {
            opcode = EMPTY;
            return;
        }

        const std::string &command = args[0];

        Commands cmdEnum = this->getCommandEnum(command);

        if (!isInitialized && cmdEnum != INITIALIZE && cmdEnum != EXIT && cmdEnum != UNKNOWN)
        {
            std::cout << "System not initialized. Please run 'initialize' command first." << std::endl;
            opcode = UNINITIALIZED;
            return;
        }

        opcode = cmdEnum;

        switch (cmdEnum)
        {
        case SCREEN:
            // check second parameter

            if (args.size() < 2)
            {
                std::cout << "Usage: screen <-s|-r|-ls> [<name>]" << std::endl;
                opcode = UNKNOWN;
                break;
            }

            if (args[1] == "-s" && args.size() >= 3)
                screenMode = 1; // create screen
            else if (args[1] == "-r" && args.size() >= 3)
                screenMode = 2; // view specific screen
            else if (args[1] == "-ls" && args.size() == 2)
                screenMode = 3; // list screens
            else if (args[1] == "-c" && args.size() >= 5)
                screenMode = 4; // custom instructions
            else
            {
                std::cout << "Unknown screen option: " << args[1] << std::endl;
                opcode = UNKNOWN;
            }

            break;

        case INITIALIZE:
        case EXIT:
        case SCHEDULER_START:
        case SCHEDULER_STOP:
        case PROCESS_SMI:
        case REPORT_UTIL:
        case UNKNOWN:
        default:
            break;
        }
    }

private:
    /**
     * Abstract the mapping of string commands to enum values.
     */
    Commands getCommandEnum(const std::string &command)
    {
        if (command == "initialize" || command == "init")
            return INITIALIZE;
        else if (command == "exit" || command == "quit")
            return EXIT;
        else if (command == "screen")
            return SCREEN;
        else if (command == "scheduler-start")
            return SCHEDULER_START;
        else if (command == "scheduler-stop")
            return SCHEDULER_STOP;
        else if (command == "process-smi")
            return PROCESS_SMI;
        else if (command == "report-util")
            return REPORT_UTIL;
        else
        {
            std::cout << "Unknown command: " << command << std::endl;
            return UNKNOWN;
        }
    }

    void loadConfig()
    {
        const std::string filepath = "config.txt";
        Config config(filepath);
        std::cout << "Configuration loaded from " << filepath << std::endl;
        std::cout << "  CPUs: " << config.getNumCpus() << "\n";
        std::cout << "  Scheduler: " << config.getSchedulerAlgorithm() << "\n";
        std::cout << "  Quantum cycles: " << config.getQuantumCycles() << "\n";
        std::cout << "  Batch process frequency: " << config.getBatchProcessFreq() << "\n";
        std::cout << "  Instruction range: " << config.getMinInstructions() << "-" << config.getMaxInstructions() << "\n";
        std::cout << "  Delays per exec: " << config.getDelayPerExec() << "\n";
        std::cout << "  Time between instructions: " << config.getTimeBetweenInstructions() << " ms\n";
    }
};