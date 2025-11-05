#include <iostream>
#include <vector>
#include "model\Config.cpp"
#include "model\CPU.cpp"
#include "handlers\ScheduleHandler.cpp"
#include "view\CLI.cpp"
#include "handlers\CommandHandler.cpp"
#include <thread>
#include <atomic>

/**  TODO: TLDR there will be 2 threads:
 *   - Main thread: handles CLI input and command parsing
 *   - Scheduler thread: handles CPU scheduling and process execution
 */

class GreggyOS
{
    std::vector<CPU> cpus;
    std::atomic<bool> isInitialized;
    std::atomic<bool> isRunning;
    std::thread schedulerThread;
    std::thread cliThread;
    CommandLineInterface cli;
    Config *config;
    std::shared_ptr<ScheduleHandler> scheduler;
    CommandHandler commandHandler;
    std::string configFilePath;
    std::mutex screenMutex;

public:
    GreggyOS(std::string configFilePath)
    {
        this->configFilePath = configFilePath;
        this->cli = CommandLineInterface();
        this->isInitialized = false;
        this->isRunning = true;

        this->config = new Config(this->configFilePath);

        // spawn threads
        this->schedulerThread = std::thread(&GreggyOS::schedulerLoop, this, std::ref(this->isRunning), std::ref(this->isInitialized));
        this->cliThread = std::thread(&GreggyOS::commandThread, this, std::ref(this->isRunning), std::ref(this->isInitialized));

        this->schedulerThread.join();
        this->cliThread.join();
    }

    void initializeConfig()
    {
        try
        {
            Config lConfig("config.txt");
            *this->config = lConfig;

            this->cli.displayMessage("");
            this->cli.displayMessage("Configuration loaded from config.txt");

            this->scheduler = std::make_shared<ScheduleHandler>(
                this->config->getSchedulerAlgorithm(),
                this->config->getQuantumCycles());

            this->cli.displayMessage("Scheduler initialized with algorithm: " + this->config->getSchedulerAlgorithm() + " and quantum cycles: " + std::to_string(this->config->getQuantumCycles()));

            // spawn CPUs based on config
            for (int i = 0; i < this->config->getNumCpus(); ++i)
                cpus.emplace_back(i, this->config->getQuantumCycles());

            this->cli.displayMessage("System initialized with configuration from config.txt");
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
    }

    void schedulerLoop(std::atomic<bool> &isRunning, std::atomic<bool> &isInitialized)
    {
        while (isRunning)
        {
            scheduler->executeSchedulingCycle(cpus);
        }
    }

    void commandThread(std::atomic<bool> &isRunning, std::atomic<bool> &isInitialized)
    {
        std::atomic<Commands> opcode(Commands::UNKNOWN);
        std::atomic<int> screenMode(-1);
        while (isRunning)
        {
            this->cli.displayMessage("");
            std::vector<std::string> userInput = splitString(this->cli.getUserInput("C:\\GreggyOS"), ' ');
            commandHandler.parseCommand(userInput, isRunning, isInitialized, opcode, screenMode);

            switch (opcode)
            {
            case INITIALIZE:
                if (!isInitialized)
                {
                    this->initializeConfig();
                    this->cli.displayMessage("System initialized.");
                    isInitialized = true;
                }
                else
                {
                    this->cli.displayMessage("System already initialized.");
                }
                break;
            case EXIT:
                isRunning = false;
                this->cli.displayMessage("Exiting GreggyOS...");
                break;
            case SCREEN:
            {
                std::lock_guard<std::mutex> lock(screenMutex);
                std::string screenName = "";

                switch (screenMode)
                {
                case 1: // create screen
                    if (userInput.size() < 3)
                    {
                        this->cli.displayMessage("Please provide process name to create.");
                        break;
                    }

                    screenName = userInput[2];
                    this->cli.displayMessage("Creating screen " + screenName);

                    break;
                case 2: // view specific screen
                    if (userInput.size() < 3)
                    {
                        this->cli.displayMessage("Please provide a process name to view.");
                        break;
                    }

                    screenName = userInput[2];
                    this->cli.displayMessage("Viewing screen " + screenName);
                    break;

                case 3: // list screens
                    this->cli.displayMessage("Listing all screens...");
                    break;
                }
            }
            default:
                break;
            }
        }
    }
};

int main()
{
    GreggyOS os("config.txt");
    return 0;
}