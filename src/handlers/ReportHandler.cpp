#pragma once
#include "../model/Process.cpp"
#include "../model/Config.cpp"
#include "ScreenHandler.cpp"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <memory>
#include <sstream>
#include <ctime>
#include <algorithm>

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
        if (!screenHandler) return;

        // Get all processes from ScreenHandler
        auto allProcesses = screenHandler->getAllProcesses();
        
        // Clear current lists
        runningProcesses.clear();
        
        // Categorize processes
        for (const auto& process : allProcesses) {
            if (process->isFinished()) {
                // Only add if not already in completed list
                if (std::find(completedProcesses.begin(), completedProcesses.end(), process) 
                    == completedProcesses.end()) {
                    completedProcesses.push_back(process);
                }
            } else {
                runningProcesses.push_back(process);
            }
        }
        
        updateCoresUsed();
    }

private:
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

        report << "----------------------------------------------\n";

        return report.str();
    }

public:
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
};

class ReportHandler {
private:
    Config* config;
    
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

public:
    ReportHandler(Config* configPtr) 
        : config(configPtr), 
          totalCpuActiveTime(0), 
          totalCpuIdleTime(0),
          isCpuCurrentlyActive(false),
          totalProcessesCreated(0),
          totalProcessesCompleted(0),
          coresUsed(0) {
        
        systemStartTime = std::chrono::system_clock::now();
        lastUpdateTime = systemStartTime;
        
        // Initialize cores based on config
        numCpuCores = config ? config->getNumCpus() : 1;
        coreActiveTimes.resize(numCpuCores, std::chrono::duration<double>(0));
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

private:
    /**
     * Generates the status report content (used by both screen -ls and report-util)
     */
    std::string generateStatusReport(bool includeTimestamp) {
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

        report << "----------------------------------------------\n";

        return report.str();
    }

    /**
     * Gets formatted timestamp string
     */
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(now);
        char buffer[100];
        std::strftime(buffer, sizeof(buffer), "%m/%d/%Y, %I:%M:%S %p", std::localtime(&time));
        return std::string(buffer);
    }

public:
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
};