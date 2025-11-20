#pragma once
#include "../model/CPU.cpp"
#include "../model/readyqueue/FCFS.cpp"
#include "../model/readyqueue/RoundRobin.cpp"
#include <vector>
#include <memory>
#include <atomic>
#include <string>

class ScheduleHandler
{
private:
    std::shared_ptr<ReadyQueue> readyQueue;
    std::vector<std::shared_ptr<Process>> waitingQueue;
    std::string schedulerType;
    std::atomic<unsigned long long> cpuTicks;

public:
    ScheduleHandler(const std::string &schedulerType, int timeQuantum = 5)
        : schedulerType(schedulerType)
    {
        this->cpuTicks = 0;

        // Initialize the appropriate scheduler
        if (schedulerType == "fcfs") {
            readyQueue = std::make_shared<FCFS>();
        } else if (schedulerType == "rr") {
            readyQueue = std::make_shared<RoundRobin>(timeQuantum);
        } else {
            // Default to FCFS
            readyQueue = std::make_shared<FCFS>();
        }
    }

    /**
     * Adds a process to the ready queue
     */
    void addProcess(std::shared_ptr<Process> process)
    {
        readyQueue->enqueueProcess(process);
    }

    /**
     * Assigns processes from ready queue to idle CPUs
     */
    void schedule(std::vector<CPU> &cpus)
    {
        for (auto &cpu : cpus)
        {
            // If CPU is idle and there are processes waiting
            if (cpu.isIdle() && !readyQueue->empty())
            {
                auto process = readyQueue->dequeueProcess();
                if (process != nullptr)
                {
                    cpu.addProcess(process);
                }
            }
        }
    }

    /**
     * Executes one scheduling cycle
     * - Executes instructions on all CPUs
     * - Handles process completion and preemption (for Round Robin)
     * - Increments CPU tick counter
     */
    void executeSchedulingCycle(std::vector<CPU> &cpus)
    {
        // Update sleep timers
        for (auto it = waitingQueue.begin(); it != waitingQueue.end(); ) {
            auto &p = *it;

            p->updateSleep(1);  // decrement 1 tick

            if (p->getState() == ProcessState::READY) {
                // Move back to ready queue
                readyQueue->enqueueProcess(p);
                it = waitingQueue.erase(it);
            } else {
                ++it;
            }
        }

        // Increment CPU tick counter at the start of each cycle
        cpuTicks++;

        for (auto &cpu : cpus)
        {
            if (!cpu.isIdle())
            {
                // Execute next instruction and check if process needs requeuing
                auto processToRequeue = cpu.executeNext();
                
                // If process was preempted, requeue it
                if (processToRequeue != nullptr) {
                    if (processToRequeue->getState() == ProcessState::WAITING) {
                        waitingQueue.push_back(processToRequeue);
                    }
                    else if (!processToRequeue->isFinished()) {
                        readyQueue->enqueueProcess(processToRequeue);
                    }
                }
            }

            // Assign new process if CPU became idle
            if (cpu.isIdle() && !readyQueue->empty())
            {
                auto process = readyQueue->dequeueProcess();
                if (process != nullptr)
                {
                    cpu.addProcess(process);
                }
            }
        }
    }

    /**
     * Gets the ready queue
     */
    std::shared_ptr<ReadyQueue> getReadyQueue()
    {
        return readyQueue;
    }

    /**
     * Gets the scheduler type
     */
    std::string getSchedulerType() const
    {
        return schedulerType;
    }

    /**
     * Gets the number of processes waiting in the ready queue
     */
    size_t getWaitingProcessCount()
    {
        return readyQueue->size();
    }

    /**
     * Checks if ready queue is empty
     */
    bool isReadyQueueEmpty()
    {
        return readyQueue->empty();
    }

    /**
     * Gets the current CPU tick count
     * @return Current number of CPU ticks (scheduling cycles executed)
     */
    unsigned long long getCpuTicks() const
    {
        return cpuTicks.load();
    }

    /**
     * Resets the CPU tick counter to zero
     * Useful for testing or when restarting the scheduler
     */
    void resetCpuTicks()
    {
        cpuTicks = 0;
    }

    /**
     * Gets CPU tick count as string for display
     * @return Formatted string with current tick count
     */
    std::string getCpuTicksString() const
    {
        return "CPU Ticks: " + std::to_string(cpuTicks.load());
    }
};