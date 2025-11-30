#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <thread>
#include <atomic>
#include "./headers.h"
#include "model\Config.cpp"
#include "model\CPU.cpp"
#include "handlers\ScheduleHandler.cpp"
#include "handlers\GeneratorHandler.cpp"
#include "handlers\ScreenHandler.cpp"
#include "handlers\CommandHandler.cpp"
#include "handlers\ReportHandler.cpp"
#include "view\CLI.cpp"
#include "view\misc.cpp"

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
    std::shared_ptr<Memory> memory;
    std::shared_ptr<ScheduleHandler> scheduler;
    std::shared_ptr<GeneratorHandler> processGenerator;
    std::shared_ptr<ReportHandler> reportHandler;
    std::shared_ptr<ScreenHandler> screenHandler;
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

        this->config = nullptr;
        this->reportHandler = std::make_shared<ReportHandler>(this->config);
        this->processGenerator = std::make_shared<GeneratorHandler>();

        // spawn threads
        this->schedulerThread = std::thread(&GreggyOS::schedulerLoop, this, std::ref(this->isRunning), std::ref(this->isInitialized));
        this->cliThread = std::thread(&GreggyOS::commandThread, this, std::ref(this->isRunning), std::ref(this->isInitialized));
    }

    ~GreggyOS()
    {
        if (schedulerThread.joinable())
            schedulerThread.join();

        this->cli.displayMessage("Goodbye!");
        if (cliThread.joinable())
            cliThread.join();

        delete config;
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
            this->config = std::make_unique<Config>(lConfig).release();

            this->cli.displayMessage("Configuration loaded from config.txt");

            // Initialize memory inside scheduler
            this->memory = std::make_shared<Memory>(this->config, "backing_store.txt");

            this->scheduler = std::make_shared<ScheduleHandler>(
                memory,
                this->config->getSchedulerAlgorithm(),
                this->config->getQuantumCycles(),
                this->config);

            this->cli.displayMessage(
                "Scheduler initialized with algorithm: " + this->config->getSchedulerAlgorithm() +
                (this->config->getSchedulerAlgorithm() == "rr"
                     ? "with time quantum: " + std::to_string(this->config->getQuantumCycles())
                     : ""));

            // Initialize report handler
            this->reportHandler = std::make_shared<ReportHandler>(this->config);

            // Initialize screen handler
            this->screenHandler = std::make_shared<ScreenHandler>(
                this->config,
                this->processGenerator.get(),
                this->scheduler.get(),
                this->reportHandler.get());

            // Link report handler with screen handler
            this->reportHandler->setScreenHandler(this->screenHandler.get());

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
        if (currentTick - lastBatchTick >= batchFreq && scheduler->getNumActiveProcesses() < 10)
        {
            auto newProcess = processGenerator->generateProcess(*config, memory);
            scheduler->addProcess(newProcess);
            if (screenHandler)
                screenHandler->registerProcess(newProcess);
            lastBatchTick = currentTick;
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
        reportHandler->generateStatusReport(true);
    }

    /**
     * Provides a summary of memory usage and CPU ticks
     */
    std::string vmstat()
    {
        std::stringstream msgStream;

        msgStream
            << "------------------------------------------------------------\n"
            << "                   GreggyOS Memory Report                   \n"
            << "------------------------------------------------------------\n"
            << "Total Memory:     " << memory->getTotalMemoryBytes() << " bytes\n"
            << "Free Memory:      " << memory->getFreeMemoryBytes() << " bytes\n"
            << "Used Memory:      " << memory->getUsedMemoryBytes() << " bytes\n"
            << "Active CPU ticks: " << static_cast<unsigned long long>(scheduler->getActiveTicks()) << "\n"
            << "Total CPU ticks:  " << static_cast<unsigned long long>(scheduler->getCpuTicks()) << "\n"
            << "Num Page-ins:     " << static_cast<uint64_t>(memory->getNumPagedIn()) << "\n"
            << "Num Page-outs:    " << static_cast<uint64_t>(memory->getNumPagedOut()) << "\n"
            << "------------------------------------------------------------\n";

        return msgStream.str();
    }

    std::string processsmi()
    {
        std::stringstream ss;

        // Header
        ss << "------------------------------------------------------------\n";
        ss << "                   PROCESS-SMI v01.00                        \n";
        ss << "------------------------------------------------------------\n";

        // ---------- CPU UTIL ----------
        uint64_t totalTicks = scheduler->getCpuTicks();
        uint64_t activeTicks = scheduler->getActiveTicks();
        uint64_t cpuUtil = 0;
        if (activeTicks > 0)
            cpuUtil = static_cast<uint64_t>((activeTicks * 100) / totalTicks);
        else
            cpuUtil = 0;

        // ---------- MEMORY UTIL ----------
        uint64_t usedMem = memory->getUsedMemoryBytes();
        uint64_t totalMem = memory->getTotalMemoryBytes();
        int memUtil = static_cast<int>((usedMem * 100) / totalMem);

        // Convert to MiB for display
        // double usedMiB = usedMem / (1024.0 * 1024.0);
        // double totalMiB = totalMem / (1024.0 * 1024.0);

        ss << "CPU-Util:     " << static_cast<uint64_t>(cpuUtil) << "%\n";
        ss << "Memory Usage: " << std::fixed << std::setprecision(2)
        << usedMem << "B / " << totalMem << "B\n";
        ss << "Memory Util:  " << memUtil << "%\n\n";

        ss << "============================================================\n";
        ss << "Running processes and memory usage:\n";
        ss << "------------------------------------------------------------\n";

        // ---------- PROCESS LIST ----------
        auto processes = scheduler->getAllScheduledProcesses(cpus);

        if (processes.empty())
        {
            ss << "(no running processes)\n";
        }
        else
        {
            for (auto &p : processes)
            {
                // double pmem = p->getMemoryUsage() / (1024.0 * 1024.0);
                if (p == nullptr)
                    continue;

                ss << p->getName() << "   " << std::fixed << std::setprecision(2) 
                << memory->getMemUsedByProcess(p->getPageNumbers(), p->getMemoryUsage()) << "B / "
                << p->getMemoryUsage() << "B\n";
            }
        }

        ss << "------------------------------------------------------------\n";

        return ss.str();
    }

    void commandThread(std::atomic<bool> &isRunning, std::atomic<bool> &isInitialized)
    {
        std::atomic<Commands> opcode(Commands::UNKNOWN);
        std::atomic<int> screenMode(-1);
        while (isRunning)
        {
            std::vector<std::string> userInput = splitString(this->cli.getUserInput("C:\\GreggyOS"), ' ');
            commandHandler.parseCommand(userInput, isRunning, isInitialized, opcode, screenMode);

            if (userInput.empty())
                continue;

            cli.displayMessage();
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
                if (screenHandler != nullptr && !screenHandler->writeScreenReport())
                    cli.displayMessage("Failed to generate utilization report.");
                else
                    cli.displayMessage("System not initialized. Cannot generate report.");
                break;
            case PROCESS_SMI:
                if (scheduler && memory)  // optional safety check
                    this->cli.displayMessage(this->processsmi());
                else
                    this->cli.displayMessage("System not initialized. Cannot run PROCESS_SMI.");
                break;
            case SCREEN:
            {
                std::lock_guard<std::mutex> lock(screenMutex);
                std::string screenName = "";

                if (!screenHandler)
                {
                    cli.displayMessage("System not initialized. Cannot access screens.");
                    break;
                }

                switch (screenMode)
                {
                case 1: // create screen
                {
                    if (userInput.size() < 4)
                    {
                        this->cli.displayMessage("Usage: screen -s <name> <memory_bytes>");
                        break;
                    }

                    screenName = userInput[2];
                    int memoryBytes = 0;
                    try
                    {
                        memoryBytes = std::stoi(userInput[3]);
                    }
                    catch (...)
                    {
                        this->cli.displayMessage("Invalid memory allocation: " + userInput[3]);
                        break;
                    }

                    if (screenHandler->createScreen(screenName, memoryBytes, this->memory.get()))
                    {
                        this->cli.displayMessage("Process created. Use 'screen -r " + screenName + "' to attach.");
                    }
                    break;
                }

                case 2: // view specific screen (interactive mode)
                    if (userInput.size() < 3)
                    {
                        this->cli.displayMessage("Please provide a process name to view.");
                        break;
                    }

                    screenName = userInput[2];
                    screenHandler->enterInteractiveScreen(screenName);
                    break;

                case 3: // list screens
                {
                    screenHandler->displayScreenList();
                }
                break;

                case 4: // custom instructions
                    if (userInput.size() < 5)
                    {
                        this->cli.displayMessage("Usage: screen -c <name> <memory_size> \"<instructions>\"");
                        break;
                    }

                    screenName = userInput[2];
                    int memorySize = 0;
                    try
                    {
                        memorySize = std::stoi(userInput[3]);
                    }
                    catch (const std::exception &)
                    {
                        this->cli.displayMessage("Invalid memory size. Please provide a numeric value.");
                        break;
                    }

                    auto assembleTokens = [](const std::vector<std::string> &tokens, size_t startIndex)
                    {
                        std::ostringstream oss;
                        for (size_t i = startIndex; i < tokens.size(); ++i)
                        {
                            if (i > startIndex)
                            {
                                oss << ' ';
                            }
                            oss << tokens[i];
                        }
                        return oss.str();
                    };

                    std::string rawInstructions = trimString(assembleTokens(userInput, 4));
                    if (rawInstructions.size() >= 2 && rawInstructions.front() == '"' && rawInstructions.back() == '"')
                    {
                        rawInstructions = rawInstructions.substr(1, rawInstructions.size() - 2);
                    }

                    if (rawInstructions.empty())
                    {
                        this->cli.displayMessage("Instruction string cannot be empty.");
                        break;
                    }

                    if (screenHandler->createCustomProcess(screenName, memorySize, this->memory.get(), rawInstructions))
                    {
                        this->cli.displayMessage("Custom process created. Use 'screen -r " + screenName + "' to attach.");
                    }
                    else
                    {
                        this->cli.displayMessage("Failed to create custom process.");
                    }
                    break;
                }
                break; // Break from SCREEN case
            }

            case VMSTAT:
                if (memory)
                    this->cli.displayMessage(this->vmstat());
                else
                    cli.displayMessage("Memory not initialized.");
                break;
#ifdef DEBUG
            case MEM_SNAPSHOT:
                if (memory)
                    cli.displayMessage(memory->getMemorySnapshot());
                else
                    cli.displayMessage("Memory not initialized.");
                break;

            case FORCE_FLUSH:
                if (memory)
                {
                    memory->flushAllPagesToBackingStore();
                    cli.displayMessage("All pages flushed to backing store.");
                }
                else
                    cli.displayMessage("Memory not initialized.");
                break;
#endif

            default:
                break;
            }
            cli.displayMessage();
        }
    }
};

int main()
{
    GreggyOS os("config.txt");
    return 0;
}