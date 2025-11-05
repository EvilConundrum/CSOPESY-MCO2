#pragma once
#include "../model/CPU.cpp"
#include "../model/readyqueue/FCFS.cpp"
#include "../model/readyqueue/RoundRobin.cpp"
#include <vector>
#include <memory>

class ScheduleHandler
{
private:
    std::shared_ptr<ReadyQueue> readyQueue;
    std::string schedulerType;

public:
    ScheduleHandler(const std::string &schedulerType, int timeQuantum = 5)
        : schedulerType(schedulerType)
    {
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
     */
    void executeSchedulingCycle(std::vector<CPU> &cpus)
    {
        for (auto &cpu : cpus)
        {
            if (!cpu.isIdle())
            {
                // Execute next instruction and check if process needs requeuing
                auto processToRequeue = cpu.executeNext();
                
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
};