#pragma once
#include <string>
#include <queue>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <mutex>
#include "Instruction.cpp"

enum class ProcessState { READY, RUNNING, WAITING, FINISHED };

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

    // Variable storage (persistent process memory)
    std::unordered_map<std::string, uint16_t> variables;

    // Process logs (capture instruction outputs)
    std::vector<std::string> logs;
    std::mutex logMutex;

public:
    Process(std::string pid, int totalInstructions = 0)
    {
        this->PID = pid;
        this->state = ProcessState::READY;
        this->currentInstructionLine = 0;
        this->totalInstructions = totalInstructions;
        this->memoryRequired = 0;
        this->creationTime = std::chrono::system_clock::now();
        this->instructions.reserve(totalInstructions);
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
    bool executeNextInstruction()
    {
        if (currentInstructionLine >= instructions.size()) {
            this->state = ProcessState::FINISHED;
            this->endTime = std::chrono::system_clock::now();
            return false;
        }

        // Execute instruction silently (no console output) with logging
        auto logCallback = [this](const std::string& logEntry) {
            this->addLog(logEntry);
        };
        
        // Pass false for printToConsole to suppress console output
        int sleepTime = instructions[currentInstructionLine].execute(variables, logCallback, false);
        
        if (sleepTime > 0) {
            // Simulate sleep by just returning (actual sleep handled by CPU scheduler)
            return true;
        }

        currentInstructionLine++;
        return true;
    }

    /**
     * Gets the current instruction without advancing
     */
    Instruction getCurrentInstruction() const
    {
        if (currentInstructionLine < instructions.size()) {
            return instructions[currentInstructionLine];
        }
        return Instruction();
    }

    /**
     * Checks if the process has remaining instructions
     */
    bool hasInstructions() const
    {
        return currentInstructionLine < instructions.size();
    }

    /**
     * Gets the total number of instructions
     */
    int getTotalInstructions() const
    {
        return totalInstructions;
    }

    /**
     * Gets the current instruction line number
     */
    int getCurrentLine() const
    {
        return currentInstructionLine;
    }

    /**
     * Gets the PID of the process
     */
    std::string getPID() const
    {
        return PID;
    }

    /**
     * Gets the current state of the process
     */
    ProcessState getState() const
    {
        return state;
    }

    /**
     * Sets the state of the process
     */
    void setState(ProcessState newState)
    {
        this->state = newState;

        // Set start time when process begins running
        if (newState == ProcessState::RUNNING && currentInstructionLine == 0) {
            this->startTime = std::chrono::system_clock::now();
        }
        // Set end time when process finishes
        else if (newState == ProcessState::FINISHED) {
            this->endTime = std::chrono::system_clock::now();
        }
    }

    /**
     * Checks if the process is finished
     */
    bool isFinished() const
    {
        return state == ProcessState::FINISHED || currentInstructionLine >= totalInstructions;
    }

    /**
     * Gets the creation time of the process
     */
    std::chrono::system_clock::time_point getCreationTime() const
    {
        return creationTime;
    }

    /**
     * Gets formatted creation time as string
     */
    std::string getCreationTimeStr() const
    {
        std::time_t time = std::chrono::system_clock::to_time_t(creationTime);
        char buffer[26];
        ctime_s(buffer, sizeof(buffer), &time);
        std::string timeStr(buffer);
        // Remove newline from ctime output
        if (!timeStr.empty() && timeStr.back() == '\n') {
            timeStr.pop_back();
        }
        return timeStr;
    }

    /**
     * Resets the process to initial state
     */
    void reset()
    {
        currentInstructionLine = 0;
        state = ProcessState::READY;
        variables.clear(); // 🔹 Reset variable memory too
    }

    /**
     * Sets memory requirement for the process
     */
    void setMemoryRequired(int memory)
    {
        this->memoryRequired = memory;
    }

    /**
     * Gets memory requirement for the process
     */
    int getMemoryRequired() const
    {
        return memoryRequired;
    }

    // 🔹 Variable access helpers
    const std::unordered_map<std::string, uint16_t>& getVariables() const
    {
        return variables;
    }

    uint16_t getVariable(const std::string& name) const
    {
        auto it = variables.find(name);
        if (it != variables.end()) return it->second;
        return 0;
    }

    void setVariable(const std::string& name, uint16_t value)
    {
        variables[name] = value;
    }

    /**
     * Adds a log entry to the process
     */
    void addLog(const std::string& logEntry)
    {
        std::lock_guard<std::mutex> lock(logMutex);
        logs.push_back(logEntry);
    }

    /**
     * Gets all process logs
     */
    std::vector<std::string> getLogs() const
    {
        return logs;
    }

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
        if (currentInstructionLine >= instructions.size()) {
            return "No instruction (process finished)";
        }
        
        const auto& instr = instructions[currentInstructionLine];
        std::string result = instr.getCommand();
        
        auto args = instr.getArgs();
        for (const auto& arg : args) {
            result += " " + arg;
        }
        
        return result;
    }
};