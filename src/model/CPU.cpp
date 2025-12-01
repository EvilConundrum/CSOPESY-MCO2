#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <mutex>
#include "readyqueue/ReadyQueue.cpp"
#include "Process.cpp"
#include "WaitingQueue.cpp"

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
        if (this->currProcess != nullptr)
        {
            this->currProcess->setAssignedCore(this->coreID);
        }
    }

    /**
     * Executes the next instruction of the current process
     * Returns the process if it needs to be re-queued (time quantum expired)
     * Returns nullptr if process continues or is finished
     */
    std::shared_ptr<Process> executeNext(WaitingQueue *waitingQueue = nullptr, Memory *memory = nullptr, uint64_t currentTick = 0)
    {
        if (this->currProcess != nullptr && this->currProcess->hasInstructions())
        {
            // Set process state to running
            if (this->currProcess->getState() != ProcessState::RUNNING)
            {
                this->currProcess->setState(ProcessState::RUNNING);
            }

            // Execute next instruction
            bool executed = this->currProcess->executeNextInstruction(memory, currentTick);

            if (this->currProcess != nullptr && this->currProcess->isSleeping())
            {
                if (this->currProcess != nullptr && (this->currProcess->isFinished() || this->currProcess->isTerminated()))
                    completeCurrentProcess(memory, this->currProcess->isTerminated());
                else if (waitingQueue != nullptr)
                {
                    waitingQueue->enqueueProcess(this->currProcess);
                    this->currProcess->setAssignedCore(-1);
                    this->currProcess = nullptr;
                    this->timeLeft = this->timeQuantum;
                }
                else
                {
                    this->currProcess->wakeFromSleep();
                }
                return nullptr;
            }

            if (executed)
            {
                if (this->timeQuantum > 0)
                {
                    if (this->timeLeft > 0)
                    {
                        this->timeLeft--;
                    }

                    if (this->timeLeft <= 0)
                    {
                        this->timeLeft = this->timeQuantum;

                        if (this->currProcess != nullptr)
                        {
                            this->currProcess->setAssignedCore(-1);
                        }

                        // check if process is already done
                        if (this->currProcess->isFinished() || this->currProcess->isTerminated())
                        {
                            completeCurrentProcess(memory, this->currProcess->isTerminated());
                            return nullptr;
                        }

                        this->currProcess->setState(ProcessState::READY);

                        auto processToRequeue = this->currProcess;
                        this->currProcess = nullptr;
                        return processToRequeue;
                    }
                }
                else
                {
                    this->currProcess->setState(ProcessState::READY);
                }
            }
        }

        // Check if process is finished
        if (this->currProcess != nullptr && this->currProcess->isFinished())
        {
            completeCurrentProcess(memory);
        }

        // check if process was terminated due to memory violation
        if (this->currProcess != nullptr && this->currProcess->isTerminated())
        {
            completeCurrentProcess(memory, true);
        }

        return nullptr;
    }

    /**
     * Checks if there are remaining processes to execute
     */
    bool
    hasRemainingProcesses() const
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
        if (this->currProcess != nullptr)
        {
            this->currProcess = nullptr;
        }
        if (process != nullptr)
        {
            process->setAssignedCore(-1);
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

private:
    void completeCurrentProcess(Memory *memory, bool isTerminated = false)
    {
        if (this->currProcess == nullptr)
        {
            return;
        }

        if (memory != nullptr)
        {
            std::vector<int> allocatedPages = this->currProcess->getPageNumbers();
            for (int pageNumber : allocatedPages)
                memory->releasePage(pageNumber);
        }

        ProcessState ps = isTerminated ? ProcessState::TERMINATED : ProcessState::FINISHED;
        this->currProcess->setState(ps);

        this->currProcess->setAssignedCore(-1);
        std::string logMsg = "Process " + this->currProcess->getPID() + " finished on core " + std::to_string(this->coreID);
        this->logs.push_back(logMsg);
        this->currProcess = nullptr;
        this->timeLeft = this->timeQuantum;
    }
};