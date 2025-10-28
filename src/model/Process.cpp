#include <string>
#include <queue>
#include <vector>
#include <chrono>
#include <ctime>
#include "Instruction.cpp"

enum class ProcessState { READY, RUNNING, WAITING, FINISHED };

class Process
{
    std::string PID;
    std::vector<Instruction> instructions;
    ProcessState state;
    
    // Process metadata
    int currentInstructionLine;
    int totalInstructions;
    
    // Timing information
    std::chrono::system_clock::time_point creationTime;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    
    // Memory information (for future use)
    int memoryRequired;

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
        if (totalInstructions == 0) {
            totalInstructions = instructions.size();
        }
    }

    /**
     * Executes the next instruction in the process
     * Returns true if instruction was executed, false if no more instructions
     */
    bool executeNextInstruction()
    {
        if (currentInstructionLine >= instructions.size()) {
            this->state = ProcessState::FINISHED;
            this->endTime = std::chrono::system_clock::now();
            return false;
        }

        // Execute the instruction
        instructions[currentInstructionLine].execute();
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
     * Resets the process to initial state (for testing purposes)
     */
    void reset()
    {
        currentInstructionLine = 0;
        state = ProcessState::READY;
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
};