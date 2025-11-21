#pragma once
#include <vector>
#include <mutex>
#include <memory>
#include <algorithm>
#include <utility>
#include "Process.cpp"

class WaitingQueue {
private:
    struct WaitingEntry {
        std::shared_ptr<Process> process;
        unsigned long long order;
    };

    std::vector<WaitingEntry> processes;
    std::mutex queueMutex;
    unsigned long long nextOrder;

public:
    WaitingQueue() : nextOrder(0) {}
    
    ~WaitingQueue() {
        clear();
    }
    
    void enqueueProcess(std::shared_ptr<Process> process, int sleepTicks = -1) {
        if (!process) {
            return;
        }

        // Allow explicit ticks (tests) or rely on process state when called from CPU
        if (sleepTicks >= 0) {
            process->beginSleep(sleepTicks);
        }

        if (!process->isSleeping()) {
            return;
        }

        std::lock_guard<std::mutex> lock(queueMutex);
        processes.push_back({process, nextOrder++});
    }
    
    std::shared_ptr<Process> dequeueProcess() {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (processes.empty()) return nullptr;
        
        auto entry = processes.front();
        processes.erase(processes.begin());
        return entry.process;
    }
    
    bool empty() {
        std::lock_guard<std::mutex> lock(queueMutex);
        return processes.empty();
    }
    
    size_t size() {
        std::lock_guard<std::mutex> lock(queueMutex);
        return processes.size();
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(queueMutex);
        processes.clear();
    }
    
    std::shared_ptr<Process> peek() {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (processes.empty()) return nullptr;
        return processes.front().process;
    }
    
    std::vector<std::shared_ptr<Process>> getAllProcesses() {
        std::lock_guard<std::mutex> lock(queueMutex);
        std::vector<std::shared_ptr<Process>> result;
        result.reserve(processes.size());
        for (const auto& entry : processes) {
            result.push_back(entry.process);
        }
        return result;
    }
    
    bool contains(std::shared_ptr<Process> process) {
        if (process == nullptr) return false;
        
        std::lock_guard<std::mutex> lock(queueMutex);
        for (const auto& entry : processes) {
            if (entry.process == process || 
                (entry.process && entry.process->getPID() == process->getPID())) {
                return true;
            }
        }
        
        return false;
    }

    /**
     * Advances the waiting queue timers by the specified number of ticks.
     * Returns any processes whose sleep timers reached zero this cycle.
     */
    std::vector<std::shared_ptr<Process>> advanceTicks(int ticks = 1) {
        if (ticks <= 0) {
            ticks = 1;
        }

        std::lock_guard<std::mutex> lock(queueMutex);
        if (processes.empty()) {
            return {};
        }

        std::vector<size_t> readyIndices;
        std::vector<std::pair<unsigned long long, std::shared_ptr<Process>>> readyEntries;
        for (size_t i = 0; i < processes.size(); ++i) {
            auto& entry = processes[i];
            if (!entry.process || entry.process->tickSleep(ticks)) {
                readyIndices.push_back(i);
                readyEntries.push_back({entry.order, entry.process});
            }
        }

        if (readyIndices.empty()) {
            return {};
        }

        // Remove ready entries in reverse to keep indices valid
        for (auto it = readyIndices.rbegin(); it != readyIndices.rend(); ++it) {
            processes.erase(processes.begin() + *it);
        }

        std::sort(readyEntries.begin(), readyEntries.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.first < rhs.first;
        });

        std::vector<std::shared_ptr<Process>> ready;
        ready.reserve(readyEntries.size());
        for (const auto& entry : readyEntries) {
            ready.push_back(entry.second);
        }

        return ready;
    }
};