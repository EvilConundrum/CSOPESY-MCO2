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
     * Executes the next process in the CPU's process list
     */
    void executeNext()
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
                    this->timeLeft = this->timeQuantum;
                    this->currProcess->setState(ProcessState::READY);
                    this->currProcess = nullptr;
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
     */
    void clearCurrentProcess()
    {
        if (this->currProcess != nullptr) {
            this->currProcess = nullptr;
        }
        this->timeLeft = this->timeQuantum;
    }
};