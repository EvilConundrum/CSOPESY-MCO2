#pragma once
#include "../model/CPU.cpp"
#include "../model/readyqueue/FCFS.cpp"
#include "../model/readyqueue/RoundRobin.cpp"
#include "../model/WaitingQueue.cpp"
#include <vector>
#include <memory>
#include <atomic>
#include <string>

class ScheduleHandler
{
private:
    std::shared_ptr<ReadyQueue> readyQueue;
    std::string schedulerType;
    std::atomic<unsigned long long> cpuTicks;
    std::shared_ptr<WaitingQueue> waitingQueue;

public:
    ScheduleHandler(const std::string &schedulerType, int timeQuantum = 5)
        : schedulerType(schedulerType)
    {
        this->cpuTicks = 0;
        this->waitingQueue = std::make_shared<WaitingQueue>();

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
        // Increment CPU tick counter at the start of each cycle
        cpuTicks++;

        releaseSleepingProcesses();

        for (auto &cpu : cpus)
        {
            if (!cpu.isIdle())
            {
                // Execute next instruction and check if process needs requeuing
                auto processToRequeue = cpu.executeNext(waitingQueue.get());
                
                // If process was preempted, requeue it
                if (processToRequeue != nullptr && !processToRequeue->isFinished())
                {
                    if (schedulerType == "rr")
                    {
                        // Re-add to ready queue for Round Robin
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
        
        // sleep for 100 ms to simulate time between cycles
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    /**
     * Gets the ready queue
     */
    std::shared_ptr<ReadyQueue> getReadyQueue()
    {
        return readyQueue;
    }

    std::shared_ptr<WaitingQueue> getWaitingQueue()
    {
        return waitingQueue;
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

private:
    void releaseSleepingProcesses()
    {
        if (!waitingQueue) {
            return;
        }

        auto readyProcesses = waitingQueue->advanceTicks();
        for (auto& process : readyProcesses)
        {
            if (!process) {
                continue;
            }

            process->wakeFromSleep();

            if (!process->isFinished()) {
                readyQueue->enqueueProcess(process);
            }
        }
    }
};