#include <iostream>
#include <vector>
#include <string>
#include <mutex>
#include "readyqueue/ReadyQueue.cpp"
#include "Process.cpp"

class CPU
{
    int coreID;
    std::shared_ptr<Process> currProcess;
    std::vector<std::string> logs;

    // Round robin scheduling parameters
    int timeQuantum;
    int timeLeft;

public:
    CPU(int coreID, int timeQuantum)
    {
        this->coreID = coreID;
        this->currProcess = nullptr;
        this->timeQuantum = timeQuantum;
        this->timeLeft = timeQuantum;
    }

    /**
     * Adds a process to the CPU's process list
     */
    void addProcess(std::shared_ptr<Process> process)
    {
        this->currProcess = process;
    }

    /**
     * Executes the next instruction of the current process
     * Returns the process if it needs to be re-queued (time quantum expired)
     * Returns nullptr if process continues or is finished
     */
    std::shared_ptr<Process> executeNext()
    {
        if (this->currProcess != nullptr && this->currProcess->hasInstructions())
        {
            // Set process state to running
            if (this->currProcess->getState() != ProcessState::RUNNING) {
                this->currProcess->setState(ProcessState::RUNNING);
            }

            // Execute next instruction
            bool executed = this->currProcess->executeNextInstruction();
            
            if (executed) {
                // Handle round robin time quantum
                if (this->timeLeft > 0)
                {
                    this->timeLeft--;
                }
                else
                {
                    // Time quantum expired - need to requeue process
                    this->timeLeft = this->timeQuantum;
                    this->currProcess->setState(ProcessState::READY);
                    
                    // Return process for re-queueing
                    auto processToRequeue = this->currProcess;
                    this->currProcess = nullptr;
                    return processToRequeue;
                }
            }

            // Check if process is finished
            if (this->currProcess != nullptr && this->currProcess->isFinished())
            {
                this->currProcess->setState(ProcessState::FINISHED);
                std::string logMsg =    "Process " 
                                        + this->currProcess->getPID() 
                                        + " finished on core " 
                                        + std::to_string(this->coreID);
                this->logs.push_back(logMsg);
                this->currProcess = nullptr;
                this->timeLeft = this->timeQuantum;
            }
        }
        
        return nullptr;
    }

    /**
     * Executes one instruction without returning preempted process
     * (For simpler execution flow)
     */
    void executeInstruction()
    {
        if (this->currProcess != nullptr && this->currProcess->hasInstructions())
        {
            // Set process state to running
            if (this->currProcess->getState() != ProcessState::RUNNING) {
                this->currProcess->setState(ProcessState::RUNNING);
            }

            // Execute next instruction
            this->currProcess->executeNextInstruction();
            
            // Decrement time quantum
            if (this->timeLeft > 0) {
                this->timeLeft--;
            }

            // Check if process is finished
            if (this->currProcess->isFinished())
            {
                this->currProcess->setState(ProcessState::FINISHED);
                std::string logMsg =    "Process " 
                                        + this->currProcess->getPID() 
                                        + " finished on core " 
                                        + std::to_string(this->coreID);
                this->logs.push_back(logMsg);
                this->currProcess = nullptr;
                this->timeLeft = this->timeQuantum;
            }
        }
    }

    /**
     * Checks if there are remaining processes to execute
     */
    bool hasRemainingProcesses() const
    {
        return this->currProcess != nullptr && this->currProcess->hasInstructions();
    }

    /**
     * Checks if the CPU is idle (no current process)
     */
    bool isIdle() const
    {
        return this->currProcess == nullptr;
    }

    /**
     * Gets the current process
     */
    std::shared_ptr<Process> getCurrentProcess() const
    {
        return this->currProcess;
    }

    /**
     * Gets the core ID
     */
    int getCoreID() const
    {
        return this->coreID;
    }

    /**
     * Clears the current process (used when preempting)
     * Returns the removed process
     */
    std::shared_ptr<Process> clearCurrentProcess()
    {
        auto process = this->currProcess;
        if (this->currProcess != nullptr) {
            this->currProcess = nullptr;
        }
        this->timeLeft = this->timeQuantum;
        return process;
    }

    /**
     * Checks if time quantum has been exhausted
     */
    bool isTimeQuantumExpired() const
    {
        return this->timeLeft <= 0;
    }

    /**
     * Resets the time quantum
     */
    void resetTimeQuantum()
    {
        this->timeLeft = this->timeQuantum;
    }

    /**
     * Gets the remaining time quantum
     */
    int getTimeLeft() const
    {
        return this->timeLeft;
    }

    /**
     * Gets all logs
     */
    std::vector<std::string> getLogs() const
    {
        return this->logs;
    }

    /**
     * Clears all logs
     */
    void clearLogs()
    {
        this->logs.clear();
    }
};