#pragma once
#include <string>
#include <queue>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <mutex>
#include <algorithm>
#include "Instruction.cpp"

enum class ProcessState
{
    READY,
    RUNNING,
    WAITING,
    FINISHED,
    TERMINATED
};

class Process
{
    // Basic process info
    std::string PID;
    std::vector<Instruction> instructions;
    ProcessState state;
    int sleepTimeRemaining;

    // Instruction tracking
    int currentInstructionLine;
    int totalInstructions;

    // Timing info
    std::chrono::system_clock::time_point creationTime;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;

    // Memory usage info (for later expansion)
    int memoryRequired;

    // variable stuff
    // this holds variable name and their corresponding memory location
    std::unordered_map<std::string, uint16_t> variables;
    // basis of setting up pages allocated to this process
    std::vector<int> allocatedPages;

    // Process logs (capture instruction outputs)
    std::vector<std::string> logs;
    std::mutex logMutex;
    int assignedCore;

public:
    Process(std::string pid, int totalInstructions = 0, int memoryRequired = 0)
    {
        this->PID = pid;
        this->state = ProcessState::READY;
        this->currentInstructionLine = 0;
        this->totalInstructions = totalInstructions;
        this->memoryRequired = memoryRequired;
        this->creationTime = std::chrono::system_clock::now();
        this->instructions.reserve(totalInstructions);
        this->assignedCore = -1;
        this->sleepTimeRemaining = 0;
    }

    // sets the pages assigned to this process
    void addPageNumber(int pageNumber) { allocatedPages.push_back(pageNumber); }
    void addPageNumbers(const std::vector<int> &pageNumbers)
    {
        for (int pageNumber : pageNumbers)
            this->addPageNumber(pageNumber);
    }

    /**
     * Adds an instruction to the process
     */
    void addInstruction(Instruction instruction)
    {
        this->instructions.push_back(instruction);
        totalInstructions = instructions.size();
    }

    /**
     * Executes the next instruction in the process
     * Returns true if instruction was executed, false if finished
     */
    bool executeNextInstruction(Memory *memory, uint64_t currentTick)
    {
        if (currentInstructionLine >= instructions.size())
        {
            this->state = ProcessState::FINISHED;
            this->endTime = std::chrono::system_clock::now();
            return false;
        }

        // Execute instruction silently (no console output) with logging
        auto logCallback = [this](const std::string &logEntry)
        {
            this->addLog(logEntry);
        };

        // Pass false for printToConsole to suppress console output
        int sleepTime = instructions[currentInstructionLine].execute(variables, memory, currentTick, allocatedPages, logCallback, false);

        if (sleepTime > 0)
        {
            beginSleep(sleepTime);
            return true;
        }
        else if (sleepTime == -1)
        {
            // Special case: terminate process immediately
            this->state = ProcessState::TERMINATED;
            this->endTime = std::chrono::system_clock::now();
            
            // we wont add what the attempted address was as it 
            // should have been indicated in the previous line.
            std::string terminateLog = "[TERMINATE] Process " + PID + " terminated due to memory access violation at " + this->getEndTimeStr() + ".";
            this->addLog(terminateLog);

            return false;
        }
        else
        {
            if (isFinished())
            {
                this->state = ProcessState::FINISHED;
                this->endTime = std::chrono::system_clock::now();
            }
        }

        currentInstructionLine++;
        return true;
    }

    Instruction getCurrentInstruction() const { return (currentInstructionLine < instructions.size()) ? instructions[currentInstructionLine] : Instruction(); }
    bool hasInstructions() const { return currentInstructionLine < instructions.size(); }
    int getTotalInstructions() const { return totalInstructions; }
    int getCurrentLine() const { return currentInstructionLine; }

    std::string getPID() const { return PID; }
    ProcessState getState() const { return state; }

    // TODO: DON'T FORGET TO CALL THIS WHEN PROCESS FINISHES TO REALLOCATE PAGES
    std::vector<int> getAllocatedPages() const { return allocatedPages; }

    /**
     * Sets the state of the process
     */
    void setState(ProcessState newState)
    {
        this->state = newState;

        // Set start time when process begins running
        if (newState == ProcessState::RUNNING && currentInstructionLine == 0)
        {
            this->startTime = std::chrono::system_clock::now();
        }
        // Set end time when process finishes
        else if (newState == ProcessState::FINISHED)
        {
            this->endTime = std::chrono::system_clock::now();
        }
    }

    /**
     * Checks if the process is finished
     */
    bool isFinished() const { return state == ProcessState::FINISHED || currentInstructionLine >= totalInstructions; }

    /**
     * Gets the creation time of the process
     */
    std::chrono::system_clock::time_point getCreationTime() const { return creationTime; }

    std::string getCreationTimeStr() const { return formatTimePoint(creationTime); }
    std::string getStartTimeStr() const { return formatTimePoint(startTime); }
    std::string getEndTimeStr() const { return formatTimePoint(endTime); }
    bool hasStarted() const { return startTime.time_since_epoch().count() != 0; }
    bool hasCompleted() const { return endTime.time_since_epoch().count() != 0; }

    /**
     * Resets the process to initial state
     */
    void reset()
    {
        currentInstructionLine = 0;
        state = ProcessState::READY;
        variables.clear(); // 🔹 Reset variable memory too
        sleepTimeRemaining = 0;
    }

    /**
     * Gets memory requirement for the process
     */
    int getMemoryRequired() const { return memoryRequired; }

    /**
     * Adds a log entry to the process
     */
    void addLog(const std::string &logEntry)
    {
        std::lock_guard<std::mutex> lock(logMutex);
        logs.push_back(logEntry);
    }

    /**
     * Gets all process logs
     */
    std::vector<std::string> getLogs() const { return logs; }

    /**
     * Clears all process logs
     */
    void clearLogs()
    {
        std::lock_guard<std::mutex> lock(logMutex);
        logs.clear();
    }

    /**
     * Gets a string representation of the current instruction
     */
    std::string getCurrentInstructionStr() const
    {
        if (currentInstructionLine >= instructions.size())
            return "No instruction (process finished)";

        const auto &instr = instructions[currentInstructionLine];
        std::string result = instr.getCommand();

        auto args = instr.getArgs();
        for (const auto &arg : args)
            result += " " + arg;

        return result;
    }

    void setAssignedCore(int coreId) { assignedCore = coreId; }
    int getAssignedCore() const { return assignedCore; }
    bool isSleeping() const { return sleepTimeRemaining > 0 && state == ProcessState::WAITING; }
    int getSleepTimeRemaining() const { return sleepTimeRemaining; }

    void beginSleep(int ticks)
    {
        sleepTimeRemaining = std::max(0, ticks);
        if (sleepTimeRemaining > 0)
            setState(ProcessState::WAITING);
    }

    bool tickSleep(int ticks = 1)
    {
        if (sleepTimeRemaining <= 0)
            return true;

        sleepTimeRemaining -= std::max(1, ticks);
        if (sleepTimeRemaining <= 0)
        {
            sleepTimeRemaining = 0;
            return true;
        }
        return false;
    }

    void wakeFromSleep()
    {
        sleepTimeRemaining = 0;
        setState(isFinished() ? ProcessState::FINISHED : ProcessState::READY);
    }

private:
    std::string formatTimePoint(const std::chrono::system_clock::time_point &timePoint) const
    {
        if (timePoint.time_since_epoch().count() == 0)
        {
            return "N/A";
        }

        std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
        char buffer[26];
        ctime_s(buffer, sizeof(buffer), &time);
        std::string timeStr(buffer);
        if (!timeStr.empty() && timeStr.back() == '\n')
        {
            timeStr.pop_back();
        }
        return timeStr;
    }
};