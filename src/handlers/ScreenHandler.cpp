#pragma once
#include "../model/Process.cpp"
#include "../model/Config.cpp"
#include "GeneratorHandler.cpp"
#include "ScheduleHandler.cpp"
#include "ReportHandler.cpp"
#include "../model/InstructionParser.cpp"
#include <map>
#include <memory>
#include <mutex>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <unordered_set>
#include <cstdlib>
#include <chrono>
#include <ctime>

class ScreenHandler
{
private:
    std::map<std::string, std::shared_ptr<Process>> processes;
    std::vector<std::shared_ptr<Process>> finishedProcesses;
    std::vector<std::shared_ptr<Process>> terminatedProcesses;
    std::unordered_set<const Process *> archivedProcessPointers;
    std::mutex processesMutex;
    Config *config;
    GeneratorHandler *generator;
    ScheduleHandler *scheduleHandler;
    ReportHandler *reportHandler;

    static constexpr size_t MAX_FINISHED_HISTORY = 10;
    uint64_t numFinishedProcesses = 0;
    uint64_t numTerminatedProcesses = 0;

    struct ProcessDisplayInfo
    {
        std::string pid;
        ProcessState state;
        int currentLine;
        int totalInstructions;
        std::string currentInstruction;
        std::string timestamp;
        std::string finishedTimestamp;
        int assignedCore;
    };

    struct ScreenSummary
    {
        double cpuUtilization = 0.0;
        int coresUsed = 0;
        int coresAvailable = 0;
        std::vector<ProcessDisplayInfo> activeProcesses;
        std::vector<ProcessDisplayInfo> finishedProcessHistory;
        std::vector<ProcessDisplayInfo> terminatedProcessHistory;
    };

public:
    ScreenHandler(Config *configPtr,
                  GeneratorHandler *generatorPtr,
                  ScheduleHandler *scheduleHandlerPtr,
                  ReportHandler *reportHandlerPtr)
        : config(configPtr),
          generator(generatorPtr),
          scheduleHandler(scheduleHandlerPtr),
          reportHandler(reportHandlerPtr) {}

    /**
     * Creates a new process screen (screen -s <name>)
     * Now uses GeneratorHandler to create the process!
     */
    bool createScreen(const std::string &processName, int memoryBytes, Memory *memoryPtr)
    {
        if (processName.empty())
        {
            std::cout << "Process name cannot be empty.\n";
            return false;
        }

        if (!isValidMemorySize(memoryBytes))
        {
            std::cout << "invalid memory allocation" << std::endl;
            std::cout << "Allowed sizes are powers of two between 64 and 65536 bytes." << std::endl;
            return false;
        }

        if (generator == nullptr || config == nullptr)
        {
            std::cout << "Screen subsystem not initialized.\n";
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(processesMutex);
            cleanupFinishedProcessesLocked();
            if (processes.find(processName) != processes.end())
            {
                std::cout << "Process " << processName << " already exists.\n";
                return false;
            }
        }

        auto process = generator->generateProcessWithName(*config, processName, memoryPtr, memoryBytes);

        {
            std::lock_guard<std::mutex> lock(processesMutex);
            processes[processName] = process;
        }

        if (scheduleHandler)
        {
            scheduleHandler->addProcess(process);
        }

        if (reportHandler)
        {
            reportHandler->recordProcessStart(process);
        }

        std::cout << "Process " << processName << " created with "
                  << process->getTotalInstructions() << " instructions and "
                  << memoryBytes << " bytes allocated.\n";
        return true;
    }

    /**
     * Creates a process using user-defined instructions (screen -c)
     */
    bool createCustomProcess(const std::string &processName,
                             int memorySize, Memory *memoryPtr,
                             const std::string &instructionBlob)
    {
        if (processName.empty())
        {
            std::cout << "Process name cannot be empty.\n";
            return false;
        }

        if (instructionBlob.empty())
        {
            std::cout << "Instruction list cannot be empty.\n";
            return false;
        }

        if (memorySize <= 0)
        {
            std::cout << "Process memory size must be greater than zero.\n";
            return false;
        }

        InstructionParser parser(instructionBlob);
        if (!parser.parse())
        {
            std::cout << "Invalid command: unable to parse instructions.\n";
            return false;
        }

        const auto &parsedInstructions = parser.getInstructions();
        if (parsedInstructions.empty() || parsedInstructions.size() > 50)
        {
            std::cout << "Invalid command: instruction count must be between 1 and 50.\n";
            return false;
        }
        {
            std::lock_guard<std::mutex> lock(processesMutex);
            cleanupFinishedProcessesLocked();
        }
        if (processes.find(processName) != processes.end())
        {
            std::cout << "Process " << processName << " already exists.\n";
            return false;
        }

        auto process = std::make_shared<Process>(processName, static_cast<int>(parsedInstructions.size()), memorySize);
        process->addPageNumbers(memoryPtr->makePages(memorySize));

        for (const auto &instr : parsedInstructions)
        {
            process->addInstruction(instr);
        }

        {
            std::lock_guard<std::mutex> lock(processesMutex);
            processes[processName] = process;
        }

        if (scheduleHandler)
        {
            scheduleHandler->addProcess(process);
        }

        if (reportHandler)
        {
            reportHandler->recordProcessStart(process);
        }

        // this little string preconcatenation is to avoid multiple cout calls esp when im debugging methods in the other threads
        std::string outString = "Process " + processName + " created with " + std::to_string(parsedInstructions.size()) + " custom instructions. \n Memory allocated: " + std::to_string(memorySize) + " bytes (" + std::to_string(process->getNumPages()) + " pages allocated).\n\n";
        std::cout << outString;

        return true;
    }

    /**
     * Registers an existing process so it shows up in screen listings.
     * Used by scheduler-generated processes.
     */
    bool registerProcess(const std::shared_ptr<Process> &process, const std::string &alias = "")
    {
        if (!process)
        {
            return false;
        }

        std::string key = alias.empty() ? process->getPID() : alias;
        if (key.empty())
        {
            return false;
        }

        std::lock_guard<std::mutex> lock(processesMutex);
        cleanupFinishedProcessesLocked();

        auto it = processes.find(key);
        if (it == processes.end())
        {
            processes[key] = process;
            return true;
        }

        // Update pointer if re-registering the same key.
        it->second = process;
        return true;
    }

    /**
     * Enters interactive process screen mode (screen -r <name>)
     * Allows commands: process-smi, exit
     */
    bool enterInteractiveScreen(const std::string &processName, bool clearFirst = true)
    {
        auto process = getProcess(processName);
        if (!process)
        {
            std::cout << "Process " << processName << " not found.\n";
            return false;
        }

        if (clearFirst)
        {
            clearConsole();
        }

        std::cout << "\n=========================================\n";
        std::cout << "Entered process screen: " << processName << "\n";
        std::cout << "Commands: process-smi, exit\n";
        std::cout << "=========================================\n\n";

        displayProcessInfo(process);

        while (true)
        {
            std::cout << "root@" << processName << ":~$ ";
            std::string input;
            if (!std::getline(std::cin, input))
            {
                break;
            }

            size_t start = input.find_first_not_of(" \t");
            size_t end = input.find_last_not_of(" \t");
            if (start != std::string::npos && end != std::string::npos)
            {
                input = input.substr(start, end - start + 1);
            }
            else
            {
                input.clear();
            }

            if (input.empty())
            {
                continue;
            }

            if (input == "exit")
            {
                std::cout << "\nExiting process screen...\n";
                break;
            }
            else if (input == "process-smi")
            {
                std::cout << "\n";
                displayProcessInfo(process);
            }
            else
            {
                std::cout << "Unknown command: " << input << "\n";
                std::cout << "Available commands: process-smi, exit\n";
            }
        }

        archiveProcessIfFinished(processName, process);
        return true;
    }

    /**
     * Renders the equivalent of `screen -ls` directly to the console.
     */
    void displayScreenList()
    {
        auto summary = buildScreenSummary();
        std::cout << renderScreenSummary(summary, false);
    }

    /**
     * Writes the current utilization summary to a file (report-util).
     */
    bool writeScreenReport(const std::string &filename = "csopesy-log.txt")
    {
        auto summary = buildScreenSummary();
        auto content = renderScreenSummary(summary, true);

        std::ofstream file(filename);
        if (!file.is_open())
        {
            std::cerr << "Unable to open report file: " << filename << "\n";
            return false;
        }

        file << content;
        file.close();
        std::cout << "Report saved to " << filename << "\n";
        return true;
    }

    /**
     * Provides the formatted screen summary (used for testing/logging).
     */
    std::string generateScreenReport(bool includeTimestamp = false)
    {
        auto summary = buildScreenSummary();
        return renderScreenSummary(summary, includeTimestamp);
    }

private:
    /**
     * Helper method to display process information (for process-smi command)
     */
    void displayProcessInfo(std::shared_ptr<Process> process)
    {
        std::cout << "===================================\n";
        std::cout << "Process: " << process->getPID();
        if (process->isFinished())
        {
            std::cout << " (Finished!)";
        }
        std::cout << "\n";

        std::cout << "State: " << formatState(process->getState()) << "\n";
        std::cout << "Instruction Line: " << process->getCurrentLine()
                  << " / " << process->getTotalInstructions() << "\n";

        int core = process->getAssignedCore();
        std::cout << "Core: " << (core >= 0 ? std::to_string(core) : std::string("idle")) << "\n";

        if (process->hasStarted())
        {
            std::cout << "Started: " << process->getStartTimeStr() << "\n";
        }
        if (process->isFinished())
        {
            std::cout << "Finished: " << process->getEndTimeStr() << "\n";
        }

        std::cout << "\nCurrent instruction: " << process->getCurrentInstructionStr() << "\n";
        std::cout << "===================================\n";

        auto logs = process->getLogs();
        if (!logs.empty())
        {
            std::cout << "\n--- Process Output (last "
                      << std::min(static_cast<size_t>(20), logs.size())
                      << " entries) ---\n";

            size_t startIdx = logs.size() > 20 ? logs.size() - 20 : 0;
            for (size_t i = startIdx; i < logs.size(); ++i)
            {
                std::cout << logs[i] << "\n";
            }
            std::cout << "--- End of Output ---\n";
        }
        else
        {
            std::cout << "\n(No output yet)\n";
        }

        if (process->isFinished())
        {
            std::cout << "Finished!\n";
        }

        std::cout << "\n";
    }

    void clearConsole() const
    {
#ifdef _WIN32
        std::system("cls");
#else
        std::system("clear");
#endif
    }

    void cleanupFinishedProcessesLocked()
    {
        std::vector<std::string> toArchiveFinished;
        std::vector<std::string> toArchiveTerminated;
        for (const auto &entry : processes)
        {
            if (entry.second && entry.second->isFinished())
            {
                toArchiveFinished.push_back(entry.first);
            }
            else if (entry.second && entry.second->isTerminated())
            {
                toArchiveTerminated.push_back(entry.first);
            }
        }

        for (const auto &name : toArchiveFinished)
        {
            auto it = processes.find(name);
            if (it != processes.end())
            {
                archiveProcessLocked(name, it->second);
            }
        }
        for (const auto &name : toArchiveTerminated)
        {
            auto it = processes.find(name);
            if (it != processes.end())
            {
                archiveProcessLocked(name, it->second);
            }
        }
    }

    void archiveProcessLocked(const std::string &processName, const std::shared_ptr<Process> &process)
    {
        if (process && (process->isFinished() || process->isTerminated()))
        {
            bool inserted = archivedProcessPointers.insert(process.get()).second;
            if (inserted)
            {
                if (process->isTerminated())
                {
                    terminatedProcesses.push_back(process);
                    numTerminatedProcesses++;
                    if (reportHandler)
                    {
                        reportHandler->recordProcessTermination(process);
                    }
                }
                else if (process->isFinished())
                {
                    finishedProcesses.push_back(process);
                    if (finishedProcesses.size() > MAX_FINISHED_HISTORY)
                    {
                        auto dropped = finishedProcesses.front();
                        archivedProcessPointers.erase(dropped.get());
                        finishedProcesses.erase(finishedProcesses.begin());

                        if (reportHandler)
                        {
                            reportHandler->recordProcessCompletion(process);
                        }
                    }
                }
            }

            processes.erase(processName);
        }
    }

    void archiveProcessIfFinished(const std::string &processName, const std::shared_ptr<Process> &process)
    {
        if (!process || !process->isFinished())
        {
            return;
        }

        std::lock_guard<std::mutex> lock(processesMutex);
        archiveProcessLocked(processName, process);
    }

    ProcessDisplayInfo buildDisplayInfo(const std::shared_ptr<Process> &process) const
    {
        ProcessDisplayInfo info{};
        if (!process)
        {
            return info;
        }

        info.pid = process->getPID();
        info.state = process->getState();
        info.currentLine = process->getCurrentLine();
        info.totalInstructions = process->getTotalInstructions();
        info.currentInstruction = process->getCurrentInstructionStr();
        info.timestamp = process->hasStarted() ? process->getStartTimeStr() : process->getCreationTimeStr();
        info.finishedTimestamp = (process->isFinished() || process->isTerminated()) ? process->getEndTimeStr() : "";
        info.assignedCore = process->getAssignedCore();
        return info;
    }

    ScreenSummary buildScreenSummary()
    {
        ScreenSummary summary;
        std::lock_guard<std::mutex> lock(processesMutex);
        cleanupFinishedProcessesLocked();

        for (const auto &entry : processes)
        {
            if (entry.second)
            {
                summary.activeProcesses.push_back(buildDisplayInfo(entry.second));
            }
        }

        for (const auto &proc : finishedProcesses)
        {
            if (proc)
            {
                summary.finishedProcessHistory.push_back(buildDisplayInfo(proc));
            }
        }

        for (const auto &proc : terminatedProcesses)
        {
            if (proc)
            {
                summary.terminatedProcessHistory.push_back(buildDisplayInfo(proc));
            }
        }

        std::sort(summary.activeProcesses.begin(), summary.activeProcesses.end(),
                  [](const ProcessDisplayInfo &lhs, const ProcessDisplayInfo &rhs)
                  {
                      return lhs.pid < rhs.pid;
                  });

        std::sort(summary.finishedProcessHistory.begin(), summary.finishedProcessHistory.end(),
                  [](const ProcessDisplayInfo &lhs, const ProcessDisplayInfo &rhs)
                  {
                      return lhs.finishedTimestamp > rhs.finishedTimestamp;
                  });

        int totalCores = config ? config->getNumCpus() : 1;
        int runningCount = 0;
        for (const auto &info : summary.activeProcesses)
        {
            if (info.state == ProcessState::RUNNING)
            {
                runningCount++;
            }
        }

        summary.coresUsed = std::min(runningCount, totalCores);
        summary.coresAvailable = std::max(0, totalCores - summary.coresUsed);
        summary.cpuUtilization = totalCores > 0
                                     ? (static_cast<double>(summary.coresUsed) / static_cast<double>(totalCores)) * 100.0
                                     : 0.0;

        return summary;
    }

    std::string renderScreenSummary(const ScreenSummary &summary, bool includeTimestamp) const
    {
        std::ostringstream out;
        out << "\n========== Screen Sessions ==========" << "\n";

        if (includeTimestamp)
        {
            auto now = std::chrono::system_clock::now();
            std::time_t time = std::chrono::system_clock::to_time_t(now);
            char buffer[100];
            std::tm timeInfo{};
#ifdef _WIN32
            localtime_s(&timeInfo, &time);
#else
            std::tm *tmp = std::localtime(&time);
            if (tmp != nullptr)
            {
                timeInfo = *tmp;
            }
#endif
            std::strftime(buffer, sizeof(buffer), "%m/%d/%Y %I:%M:%S %p", &timeInfo);
            out << "Generated on: " << buffer << "\n";
        }

        out << std::fixed << std::setprecision(2);
        out << "CPU utilization: " << summary.cpuUtilization << "%\n";
        out << "Cores used: " << summary.coresUsed << "\n";
        out << "Cores available: " << summary.coresAvailable << "\n";
        out << "----------------------------------------------\n";

        out << "Running processes:\n";
        if (summary.activeProcesses.empty())
        {
            out << "  No running processes.\n";
        }
        else
        {
            for (const auto &info : summary.activeProcesses)
            {
                out << "  " << info.pid << " | " << formatState(info.state)
                    << " | Core: " << (info.assignedCore >= 0 ? std::to_string(info.assignedCore) : std::string("idle"))
                    << " | Started: " << info.timestamp << "\n";
                out << "    Instruction " << info.currentLine << "/" << info.totalInstructions
                    << " -> " << info.currentInstruction << "\n";
            }
        }

        out << "\nFinished processes:\n";
        if (summary.finishedProcessHistory.empty())
        {
            out << "  No finished processes.\n";
        }
        else
        {
            for (const auto &info : summary.finishedProcessHistory)
            {
                out << "  " << info.pid
                    << " | Finished: " << (info.finishedTimestamp.empty() ? "N/A" : info.finishedTimestamp)
                    << " | Last line " << info.currentLine << "/" << info.totalInstructions << "\n";
            }
        }

        out << "\nTerminated processes:\n";
        if (summary.terminatedProcessHistory.empty())
        {
            out << "  No terminated processes.\n";
        }
        else
        {
            for (const auto &info : summary.terminatedProcessHistory)
            {
                out << "  " << info.pid
                    << " | Terminated: " << (info.finishedTimestamp.empty() ? "N/A" : info.finishedTimestamp)
                    << " | Last line " << info.currentLine << "/" << info.totalInstructions << "\n";
            }
        }

        out << "----------------------------------------------\n";
        return out.str();
    }

    std::string formatState(ProcessState state) const
    {
        switch (state)
        {
        case ProcessState::READY:
            return "Ready";
        case ProcessState::RUNNING:
            return "Running";
        case ProcessState::WAITING:
            return "Waiting";
        case ProcessState::FINISHED:
            return "Finished";
        case ProcessState::TERMINATED:
            return "Terminated";
        default:
            return "Unknown";
        }
    }

public:
    /**
     * Gets a process by name
     * We can lookup both active and terminated processes
     * active to see the current instruction, terminated to see the reason for termination
     */
    std::shared_ptr<Process> getProcess(const std::string &processName)
    {
        std::lock_guard<std::mutex> lock(processesMutex);
        cleanupFinishedProcessesLocked();
        auto it = processes.find(processName);

        bool isFound = (it != processes.end());

        if (isFound)
        {
            return it->second;
        }
        
        // search terminated processes
        for (const auto &proc : terminatedProcesses)
        {
            if (proc && proc->getPID() == processName)
            {
                return proc;
            }
        }
        return nullptr;
    }

    /**
     * Gets all processes (for ReportHandler to use)
     */
    std::vector<std::shared_ptr<Process>> getAllProcesses()
    {
        std::lock_guard<std::mutex> lock(processesMutex);
        cleanupFinishedProcessesLocked();
        std::vector<std::shared_ptr<Process>> result;
        for (const auto &pair : processes)
        {
            result.push_back(pair.second);
        }
        for (const auto &finished : finishedProcesses)
        {
            result.push_back(finished);
        }
        return result;
    }

    /**
     * Checks if a process exists
     */
    bool processExists(const std::string &processName)
    {
        std::lock_guard<std::mutex> lock(processesMutex);
        cleanupFinishedProcessesLocked();
        return processes.find(processName) != processes.end();
    }

    /**
     * Gets the number of processes
     */
    size_t getProcessCount()
    {
        std::lock_guard<std::mutex> lock(processesMutex);
        cleanupFinishedProcessesLocked();
        return processes.size();
    }

    /**
     * Removes a process from the registry
     */
    bool removeProcess(const std::string &processName)
    {
        std::lock_guard<std::mutex> lock(processesMutex);
        auto it = processes.find(processName);
        if (it != processes.end())
        {
            processes.erase(it);
            return true;
        }
        return false;
    }

    bool isValidMemorySize(int memoryBytes) const
    {
        if (memoryBytes < 64 || memoryBytes > 65536)
        {
            return false;
        }
        return (memoryBytes & (memoryBytes - 1)) == 0;
    }
};
