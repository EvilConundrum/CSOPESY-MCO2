#pragma once
#include "ReadyQueue.cpp"

/**
 * FCFS (First Come First Serve) Scheduler
 * - Non-preemptive scheduling algorithm
 * - Processes are executed in the order they arrive
 */
class FCFS : public ReadyQueue
{
public:
    FCFS() : ReadyQueue() {}

    /**
     * FCFS uses the base enqueueProcess method
     * Processes are added to the end of the queue
     */
    void enqueueProcess(std::shared_ptr<Process> process) override {
        ReadyQueue::enqueueProcess(process);
    }

    /**
     * FCFS uses the base dequeueProcess method
     * Always returns the process at the front of the queue (earliest arrival)
     */
    std::shared_ptr<Process> dequeueProcess() override {
        return ReadyQueue::dequeueProcess();
    }

    /**
     * Get scheduler type name
     */
    std::string getSchedulerType() const {
        return "FCFS (First Come First Serve)";
    }
};