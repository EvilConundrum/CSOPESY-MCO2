#pragma once
#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include "Process.cpp"

class WaitingQueue {
private:
    std::queue<std::shared_ptr<Process>> processes;
    std::mutex queueMutex;

public:
    WaitingQueue() {}
    
    ~WaitingQueue() {
        clear();
    }
    
    void enqueueProcess(std::shared_ptr<Process> process) {
        std::lock_guard<std::mutex> lock(queueMutex);
        process->setState(ProcessState::WAITING);
        processes.push(process);
    }
    
    std::shared_ptr<Process> dequeueProcess() {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (processes.empty()) return nullptr;
        
        auto process = processes.front();
        processes.pop();
        return process;
    }
    
    bool empty() {
        std::lock_guard<std::mutex> lock(queueMutex);
        return processes.empty();
    }
    
    size_t size() {
        std::lock_guard<std::mutex> lock(queueMutex);
        return processes.size();
    }
    
    /**
     * Clears all processes from the waiting queue
     */
    void clear() {
        std::lock_guard<std::mutex> lock(queueMutex);
        while (!processes.empty()) {
            processes.pop();
        }
    }
    
    /**
     * Peeks at the next process without removing it
     */
    std::shared_ptr<Process> peek() {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (processes.empty()) return nullptr;
        return processes.front();
    }
    
    /**
     * Gets all processes currently in the waiting queue
     * Useful for debugging and monitoring I/O operations
     */
    std::vector<std::shared_ptr<Process>> getAllProcesses() {
        std::lock_guard<std::mutex> lock(queueMutex);
        std::vector<std::shared_ptr<Process>> result;
        
        auto tempQueue = processes;
        while (!tempQueue.empty()) {
            result.push_back(tempQueue.front());
            tempQueue.pop();
        }
        
        return result;
    }
    
    /**
     * Checks if a specific process is in the waiting queue
     */
    bool contains(std::shared_ptr<Process> process) {
        if (process == nullptr) return false;
        
        std::lock_guard<std::mutex> lock(queueMutex);
        
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