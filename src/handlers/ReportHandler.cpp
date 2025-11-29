#pragma once
#include "../model/Process.cpp"
#include "../model/Config.cpp"
// #include "ScreenHandler.cpp"  // Commented out to avoid circular dependency
#include <fstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <memory>
#include <sstream>
#include <ctime>
#include <algorithm>

// Forward declaration to avoid circular dependency
class ScreenHandler;

class ReportHandler {
private:
    Config* config;
    ScreenHandler* screenHandler; // Reference to ScreenHandler
    
    // CPU utilization tracking
    std::chrono::system_clock::time_point systemStartTime;
    std::chrono::duration<double> totalCpuActiveTime;
    std::chrono::duration<double> totalCpuIdleTime;
    std::chrono::system_clock::time_point lastUpdateTime;
    bool isCpuCurrentlyActive;
    
    // Process statistics
    int totalProcessesCreated;
    int totalProcessesCompleted;
    std::vector<std::shared_ptr<Process>> runningProcesses;
    std::vector<std::shared_ptr<Process>> completedProcesses;
    
    // Core statistics
    int numCpuCores;
    int coresUsed;
    std::vector<std::chrono::duration<double>> coreActiveTimes;

    std::vector<std::shared_ptr<Process>> terminatedProcesses;
    int totalProcessesTerminated = 0;

public:
    ReportHandler(Config* configPtr) 
        : config(configPtr), 
          screenHandler(nullptr),
          totalCpuActiveTime(0), 
          totalCpuIdleTime(0),
          isCpuCurrentlyActive(false),
          totalProcessesCreated(0),
          totalProcessesCompleted(0),
          coresUsed(0) {
        
        systemStartTime = std::chrono::system_clock::now();
        lastUpdateTime = systemStartTime;
        
        numCpuCores = config ? config->getNumCpus() : 1;
        coreActiveTimes.resize(numCpuCores, std::chrono::duration<double>(0));
    }

    /**
     * Sets the ScreenHandler reference (for getting process list)
     */
    void setScreenHandler(ScreenHandler* handler) {
        screenHandler = handler;
    }

    /**
     * Records when CPU becomes active
     */
    void markCpuActive() {
        auto now = std::chrono::system_clock::now();
        
        if (!isCpuCurrentlyActive) {
            totalCpuIdleTime += now - lastUpdateTime;
            isCpuCurrentlyActive = true;
        }
        
        lastUpdateTime = now;
    }

    /**
     * Records when CPU becomes idle
     */
    void markCpuIdle() {
        auto now = std::chrono::system_clock::now();
        
        if (isCpuCurrentlyActive) {
            totalCpuActiveTime += now - lastUpdateTime;
            isCpuCurrentlyActive = false;
        }
        
        lastUpdateTime = now;
    }

    /**
     * Records when a process starts running
     */
    void recordProcessStart(std::shared_ptr<Process> process) {
        totalProcessesCreated++;
        runningProcesses.push_back(process);
        markCpuActive();
        updateCoresUsed();
    }

    /**
     * Records when a process completes
     */
    void recordProcessCompletion(std::shared_ptr<Process> process) {
        totalProcessesCompleted++;
        completedProcesses.push_back(process);
        
        // Remove from running processes
        auto it = std::find(runningProcesses.begin(), runningProcesses.end(), process);
        if (it != runningProcesses.end()) {
            runningProcesses.erase(it);
        }
        
        updateCoresUsed();
        
        // If no more running processes, mark CPU as idle
        if (runningProcesses.empty()) {
            markCpuIdle();
        }
    }

    /**
     * Updates the number of cores currently being used
     */
    void updateCoresUsed() {
        coresUsed = std::min(static_cast<int>(runningProcesses.size()), numCpuCores);
    }

    /**
     * Calculates current CPU utilization percentage
     */
    double calculateCpuUtilization() {
        auto now = std::chrono::system_clock::now();
        auto totalSystemTime = now - systemStartTime;
        
        auto currentActiveTime = totalCpuActiveTime;
        if (isCpuCurrentlyActive) {
            currentActiveTime += now - lastUpdateTime;
        }
        
        if (totalSystemTime.count() == 0) {
            return 0.0;
        }
        
        return (currentActiveTime.count() / totalSystemTime.count()) * 100.0;
    }

    /**
     * Displays system status for "screen -ls" command
     */
    void displayScreenList() {
        std::cout << generateStatusReport(false);
    }

    /**
     * Generates and saves report for "report-util" command
     */
    void generateUtilizationReport(const std::string& filename = "csopesy-log.txt") {
        std::ofstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Error: Cannot create report file '" << filename << "'\n";
            return;
        }

        file << generateStatusReport(true);
        file.close();
        
        std::cout << "Report generated: " << filename << "\n";
    }

    /**
     * Syncs process lists with ScreenHandler
     * Call this before generating reports
     */
    void syncProcessLists() {
        // This method now just updates cores used since data is injected from main
        updateCoresUsed();
    }

    /**
     * Updates process lists from external source (called by main)
     */
    void updateProcessLists(const std::vector<std::shared_ptr<Process>>& allProcesses) {
        runningProcesses.clear();

        for (const auto& process : allProcesses) {
            if (process->isFinished()) {
                if (std::find(completedProcesses.begin(), completedProcesses.end(), process) == completedProcesses.end()) {
                    completedProcesses.push_back(process);
                    totalProcessesCompleted++;
                }
            } else if (process->isTerminated()) {
                if (std::find(terminatedProcesses.begin(), terminatedProcesses.end(), process) == terminatedProcesses.end()) {
                    terminatedProcesses.push_back(process);
                    totalProcessesTerminated++;
                }
            } else {
                runningProcesses.push_back(process);
            }
        }

        totalProcessesCreated = allProcesses.size();
        updateCoresUsed();
    }


    /**
     * Generates the status report content (used by both screen -ls and report-util)
     */
    std::string generateStatusReport(bool includeTimestamp) {
        // Sync with ScreenHandler first
        syncProcessLists();
        
        std::ostringstream report;
        
        // Add timestamp for file report
        if (includeTimestamp) {
            auto now = std::chrono::system_clock::now();
            std::time_t time = std::chrono::system_clock::to_time_t(now);
            char buffer[100];
            std::strftime(buffer, sizeof(buffer), "%m/%d/%Y, %I:%M:%S %p", std::localtime(&time));
            report << "CPU Utilization Report\n";
            report << "Generated on: " << buffer << "\n";
            report << "===============================================\n\n";
        }

        // CPU utilization and core information
        report << "CPU utilization: " << std::fixed << std::setprecision(2) 
               << calculateCpuUtilization() << "%\n";
        report << "Cores used: " << coresUsed << "\n";
        report << "Cores available: " << (numCpuCores - coresUsed) << "\n";
        report << "----------------------------------------------\n";

        // Running processes summary
        report << "Running processes:\n";
        if (runningProcesses.empty()) {
            report << "No running processes.\n";
        } else {
            for (const auto& process : runningProcesses) {
                report << process->getPID() << "   (" 
                       << process->getCreationTimeStr() << ")   "
                       << "Core: " << (runningProcesses.size() <= numCpuCores ? 
                                      std::distance(runningProcesses.begin(), 
                                                  std::find(runningProcesses.begin(), 
                                                          runningProcesses.end(), process)) : 0)
                       << "   " << process->getCurrentLine() 
                       << " / " << process->getTotalInstructions() << "\n";
            }
        }

        report << "\n";

        // Finished processes summary
        report << "Finished processes:\n";
        if (completedProcesses.empty()) {
            report << "No finished processes.\n";
        } else {
            // Show last 10 finished processes or all if less than 10
            int startIdx = std::max(0, static_cast<int>(completedProcesses.size()) - 10);
            for (int i = startIdx; i < completedProcesses.size(); ++i) {
                const auto& process = completedProcesses[i];
                report << process->getPID() << "   (" 
                       << process->getCreationTimeStr() << ")   "
                       << "Finished   " << process->getTotalInstructions() 
                       << " / " << process->getTotalInstructions() << "\n";
            }
            
            // If more than 10 processes, show count
            if (completedProcesses.size() > 10) {
                report << "... and " << (completedProcesses.size() - 10) 
                       << " more finished processes.\n";
            }
        }

        report << "\n";

        // Terminated processes summary
        report << "\nTerminated processes:\n";
        if (terminatedProcesses.empty()) {
            report << "No terminated processes.\n";
        } else {
            for (const auto& process : terminatedProcesses) {
                report << process->getPID() << "   (" 
                    << process->getCreationTimeStr() << ")   "
                    << "Terminated   " << process->getCurrentLine()
                    << " / " << process->getTotalInstructions() << "\n";
            }
        }

        report << "----------------------------------------------\n";

        return report.str();
    }

    void recordProcessTermination(std::shared_ptr<Process> process) {
        totalProcessesTerminated++;
        terminatedProcesses.push_back(process);

        // Remove from runningProcesses if present
        auto it = std::find(runningProcesses.begin(), runningProcesses.end(), process);
        if (it != runningProcesses.end()) {
            runningProcesses.erase(it);
        }

        updateCoresUsed();

        // If no more running processes, mark CPU idle
        if (runningProcesses.empty()) {
            markCpuIdle();
        }
    }


    // Getters for other components
    std::vector<std::shared_ptr<Process>> getRunningProcesses() const {
        return runningProcesses;
    }

    int getTotalProcessesCreated() const {
        return totalProcessesCreated;
    }

    int getTotalProcessesCompleted() const {
        return totalProcessesCompleted;
    }

    int getCoresUsed() const {
        return coresUsed;
    }

    int getCoresAvailable() const {
        return numCpuCores - coresUsed;
    }

    int getTotalCores() const {
        return numCpuCores;
    }
    std::vector<std::shared_ptr<Process>> getTerminatedProcesses() const {
        return terminatedProcesses;
    }

    int getTotalProcessesTerminated() const {
        return totalProcessesTerminated;
    }
};