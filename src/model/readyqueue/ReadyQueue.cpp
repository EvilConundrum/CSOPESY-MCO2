#include <vector>
#include <queue>
#include <mutex>
#include <string>
#include <memory>
#include "../Process.cpp"

class ReadyQueue {
    std::queue<std::shared_ptr<Process>> processes;
    std::mutex queueMutex;

public:
    ReadyQueue() {}

    /**
     * Adds a process to the ready queue
     */
    void enqueueProcess(std::shared_ptr<Process> process) {
        std::lock_guard<std::mutex> lock(queueMutex);
        processes.push(process);
    }

    /**
     * Pops the next process from the ready queue
     */
    std::shared_ptr<Process> dequeueProcess() {
        std::lock_guard<std::mutex> lock(queueMutex);

        if (processes.empty()) {
            return nullptr;
        }

        auto process = processes.front();
        processes.pop();
        
        return process;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(queueMutex);
        return processes.empty();
    }
};