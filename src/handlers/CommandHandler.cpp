#include <iostream>
#include <vector>
#include <string>
#include <atomic>
#include "../model/Config.cpp"

enum Commands
{
    UNKNOWN = -1,
    INITIALIZE,
    EXIT,
    SCREEN,
    SCHEDULER_START,
    SCHEDULER_STOP,
    REPORT_UTIL,
};

class CommandHandler
{
public:
    CommandHandler() {}

    void parseCommand(std::vector<std::string> args, std::atomic<bool> &running)
    {
        if (args.empty())
        {
            std::cout << "No command entered." << std::endl;
            return;
        }

        const std::string &command = args[0];
        args.erase(args.begin());
        Commands cmdEnum = this->getCommandEnum(command);
        
        bool initialized = false;

        if (!initialized && cmdEnum != INITIALIZE)
        {
            std::cout << "System not initialized. Please run 'initialize' command first." << std::endl;
            return;
        }

        switch (cmdEnum)
        {
        case INITIALIZE:
            loadConfig();
            initialized = true;
            break;
        case EXIT:
            running = false;
            break;
        case SCREEN:
            // TODO: Handle screen command
            break;
        case SCHEDULER_START:
            // TODO: Handle scheduler start command
            break;
        case SCHEDULER_STOP:
            // TODO: Handle scheduler stop command
            break;
        case REPORT_UTIL:
            // TODO: Handle report utility command
            break;
        case UNKNOWN:
        default:
            std::cout << "Unknown command." << std::endl;
            break;
        }
    }

private:
    /**
     * Abstract the mapping of string commands to enum values.
     */
    Commands getCommandEnum(const std::string &command)
    {
        if (command == "initialize")
            return INITIALIZE;
        else if (command == "exit")
            return EXIT;
        else if (command == "screen")
            return SCREEN;
        else if (command == "scheduler_start")
            return SCHEDULER_START;
        else if (command == "scheduler_stop")
            return SCHEDULER_STOP;
        else if (command == "report_util")
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