#include <iostream>
#include <vector>
#include <string>
#include <atomic>

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
        args.erase(args.begin()); // pops the first element
        Commands cmdEnum = this->getCommandEnum(command);
        switch (cmdEnum)
        {
        case INITIALIZE:
            // Handle initialization
            break;
        case EXIT:
            running = false;
            break;
        case SCREEN:
            // Handle screen command
            break;
        case SCHEDULER_START:
            // Handle scheduler start command
            break;
        case SCHEDULER_STOP:
            // Handle scheduler stop command
            break;
        case REPORT_UTIL:
            // Handle report utility command
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
};