#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <thread>
#include <atomic>
#include "model\Config.cpp"
#include "model\CPU.cpp"
#include "handlers\ScheduleHandler.cpp"
#include "handlers\GeneratorHandler.cpp"
#include "handlers\CommandHandler.cpp"
#include "handlers\ReportHandler.cpp"
#include "view\CLI.cpp"

/**  TODO: TLDR there will be 2 threads:
 *   - Main thread: handles CLI input and command parsing
 *   - Scheduler thread: handles CPU scheduling and process execution
 */

class GreggyOS
{
    std::vector<CPU> cpus;
    std::atomic<bool> isInitialized;
    std::atomic<bool> isRunning;
    std::atomic<bool> isSchedulerRunning;
    std::thread schedulerThread;
    std::thread cliThread;
    CommandLineInterface cli;
    Config *config;
    std::shared_ptr<ScheduleHandler> scheduler;
    std::shared_ptr<GeneratorHandler> processGenerator;
    std::shared_ptr<ReportHandler> report;
    CommandHandler commandHandler;
    std::string configFilePath;
    std::mutex screenMutex;
    unsigned long long lastBatchTick;

public:
    GreggyOS(std::string configFilePath)
    {
        this->configFilePath = configFilePath;
        this->cli = CommandLineInterface();
        this->isInitialized = false;
        this->isRunning = true;
        this->isSchedulerRunning = false;
        this->lastBatchTick = 0;

        this->config = new Config(this->configFilePath);
        this->processGenerator = std::make_shared<GeneratorHandler>();

        // spawn threads
        this->schedulerThread = std::thread(&GreggyOS::schedulerLoop, this, std::ref(this->isRunning), std::ref(this->isInitialized));
        this->cliThread = std::thread(&GreggyOS::commandThread, this, std::ref(this->isRunning), std::ref(this->isInitialized));
        this->report = std::make_shared<ReportHandler>(this->config);

        this->schedulerThread.join();
        this->cliThread.join();
    }

    void initializeConfig()
    {
        try
        {
            // Check if config file exists first
            std::ifstream configCheck("config.txt");
            if (!configCheck.good())
            {
                this->cli.displayMessage("Error: config.txt not found. Please ensure the configuration file exists.");
                return;
            }
            configCheck.close();

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
            this->cli.displayMessage("Error initializing system: " + std::string(e.what()));
        }
    }

    void schedulerLoop(std::atomic<bool> &isRunning, std::atomic<bool> &isInitialized)
    {
        while (isRunning)
        {
            // Only execute if initialized
            if (!isInitialized || scheduler == nullptr)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            // Generate new processes if scheduler is running
            if (isSchedulerRunning)
            {
                generateBatchProcessIfNeeded();
            }

            // Execute scheduling cycle
            scheduler->executeSchedulingCycle(cpus);

            // Small delay based on config (delays-per-exec)
            // This simulates the "busy-wait" between instruction executions
            if (config->getDelayPerExec() > 0)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(config->getDelayPerExec()));
            }
        }
    }

    /**
     * Generates a new batch process based on the configured frequency
     * Called automatically when scheduler is running
     */
    void generateBatchProcessIfNeeded()
    {
        unsigned long long currentTick = scheduler->getCpuTicks();
        int batchFreq = config->getBatchProcessFreq();

        // Check if it's time to generate a new process
        if (currentTick - lastBatchTick >= batchFreq)
        {
            auto newProcess = processGenerator->generateProcess(*config);
            scheduler->addProcess(newProcess);
            lastBatchTick = currentTick;

            // Optional: Log process generation (disabled to avoid console spam)
            // std::cout << "[Scheduler] Generated process: " << newProcess->getPID()
            //           << " at tick " << currentTick << std::endl;
        }
    }

    /**
     * Starts the automatic batch process generator
     */
    void startScheduler()
    {
        if (isSchedulerRunning)
        {
            cli.displayMessage("Scheduler is already running.");
            return;
        }

        isSchedulerRunning = true;
        lastBatchTick = scheduler->getCpuTicks();
        cli.displayMessage("Scheduler started. Automatic process generation enabled.");
    }

    /**
     * Stops the automatic batch process generator
     */
    void stopScheduler()
    {
        if (!isSchedulerRunning)
        {
            cli.displayMessage("Scheduler is not running.");
            return;
        }

        isSchedulerRunning = false;
        cli.displayMessage("Scheduler stopped. Automatic process generation disabled.");
    }

    /**
     * Generates and displays CPU utilization report
     * Also saves to csopesy-log.txt as per spec
     */
    void generateUtilizationReport()
    {
        report->generateStatusReport(true);
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
            
            case SCHEDULER_START:
                this->startScheduler();
                break;
            
            case SCHEDULER_STOP:
                this->stopScheduler();
                break;
            
            case REPORT_UTIL:
                this->generateUtilizationReport();
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
                break; // Break from SCREEN case
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