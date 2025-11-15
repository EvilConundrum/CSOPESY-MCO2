#pragma once
#include <vector>
#include <queue>
#include <mutex>
#include <string>
#include <memory>
#include "../Process.cpp"

class ReadyQueue {
protected:
    std::queue<std::shared_ptr<Process>> processes;
    std::mutex queueMutex;

public:
    ReadyQueue() {}

    virtual ~ReadyQueue() {
        clear();
    }

    /**
     * Adds a process to the ready queue
     */
    virtual void enqueueProcess(std::shared_ptr<Process> process) {
        std::lock_guard<std::mutex> lock(queueMutex);
        process->setState(ProcessState::READY);
        processes.push(process);
    }

    /**
     * Pops the next process from the ready queue
     */
    virtual std::shared_ptr<Process> dequeueProcess() {
        std::lock_guard<std::mutex> lock(queueMutex);

        if (processes.empty()) {
            return nullptr;
        }

        auto process = processes.front();
        processes.pop();
        
        return process;
    }

    /**
     * Checks if the ready queue is empty
     */
    virtual bool empty() {
        std::lock_guard<std::mutex> lock(queueMutex);
        return processes.empty();
    }

    /**
     * Gets the number of processes in the queue
     */
    virtual size_t size() {
        std::lock_guard<std::mutex> lock(queueMutex);
        return processes.size();
    }

    /**
     * Peeks at the next process without removing it
     */
    virtual std::shared_ptr<Process> peek() {
        std::lock_guard<std::mutex> lock(queueMutex);
        
        if (processes.empty()) {
            return nullptr;
        }
        
        return processes.front();
    }

    /**
     * Clears all processes from the queue
     */
    virtual void clear() {
        std::lock_guard<std::mutex> lock(queueMutex);
        while (!processes.empty()) {
            processes.pop();
        }
    }

    /**
     * Gets all processes currently in the queue (without removing them)
     * Useful for debugging, reporting, and testing
     */
    virtual std::vector<std::shared_ptr<Process>> getAllProcesses() {
        std::lock_guard<std::mutex> lock(queueMutex);
        std::vector<std::shared_ptr<Process>> result;
        
        // Create a copy of the queue to iterate without modifying
        auto tempQueue = processes;
        while (!tempQueue.empty()) {
            result.push_back(tempQueue.front());
            tempQueue.pop();
        }
        
        return result;
    }

    /**
     * Checks if a specific process is in the queue
     * @param process The process to search for
     * @return true if the process is found, false otherwise
     */
    virtual bool contains(std::shared_ptr<Process> process) {
        if (process == nullptr) {
            return false;
        }
        
        std::lock_guard<std::mutex> lock(queueMutex);
        
        // Create a copy of the queue to iterate without modifying
        auto tempQueue = processes;
        while (!tempQueue.empty()) {
            if (tempQueue.front() == process || 
                tempQueue.front()->getPID() == process->getPID()) {
                return true;
            }
            tempQueue.pop();
        }
        
        return false;
    }
};