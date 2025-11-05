#pragma once
#include "ReadyQueue.cpp"

/**
 * Round Robin Scheduler
 * - Preemptive scheduling algorithm
 * - Each process gets a fixed time quantum
 * - After time quantum expires, process is moved to the back of the queue
 * - Provides fair CPU time distribution among processes
 */
class RoundRobin : public ReadyQueue
{
private:
    int timeQuantum;

public:
    RoundRobin(int quantum = 5) : ReadyQueue(), timeQuantum(quantum) {}

    /**
     * Enqueues a process to the ready queue
     * In Round Robin, processes are added to the end of the queue
     */
    void enqueueProcess(std::shared_ptr<Process> process) override {
        ReadyQueue::enqueueProcess(process);
    }

    /**
     * Dequeues the next process from the ready queue
     * Returns the process at the front of the queue
     */
    std::shared_ptr<Process> dequeueProcess() override {
        return ReadyQueue::dequeueProcess();
    }

    /**
     * Re-enqueues a process that has used its time quantum
     * This is called when a process is preempted
     */
    void requeueProcess(std::shared_ptr<Process> process) {
        if (process != nullptr && !process->isFinished()) {
            enqueueProcess(process);
        }
    }

    /**
     * Sets the time quantum for the scheduler
     */
    void setTimeQuantum(int quantum) {
        this->timeQuantum = quantum;
    }

    /**
     * Gets the time quantum for the scheduler
     */
    int getTimeQuantum() const {
        return this->timeQuantum;
    }

    /**
     * Get scheduler type name
     */
    std::string getSchedulerType() const {
        return "Round Robin (Time Quantum: " + std::to_string(timeQuantum) + ")";
    }
};