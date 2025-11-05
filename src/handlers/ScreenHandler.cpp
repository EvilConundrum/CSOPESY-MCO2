#pragma once
#include "../model/Process.cpp"
#include "../model/Config.cpp"
#include "GeneratorHandler.cpp"
#include <map>
#include <memory>
#include <mutex>
#include <iostream>

class ScreenHandler {
private:
    std::map<std::string, std::shared_ptr<Process>> processes;
    std::mutex processesMutex;
    Config* config;
    GeneratorHandler* generator; // Reference to GeneratorHandler

public:
    ScreenHandler(Config* configPtr, GeneratorHandler* generatorPtr) 
        : config(configPtr), generator(generatorPtr) {}

    /**
     * Creates a new process screen (screen -s <name>)
     * Now uses GeneratorHandler to create the process!
     */
    bool createScreen(const std::string& processName) {
        std::lock_guard<std::mutex> lock(processesMutex);
        
        // Check if process already exists
        if (processes.find(processName) != processes.end()) {
            std::cout << "Process " << processName << " already exists.\n";
            return false;
        }

        // Use GeneratorHandler with custom name
        auto process = generator->generateProcessWithName(*config, processName);
        
        // Store in registry (PID already equals processName)
        processes[processName] = process;
        
        std::cout << "Process " << processName << " created with " 
                  << process->getTotalInstructions() << " instructions.\n";
        return true;
    }

    /**
     * Displays a specific process screen (screen -r <name>)
     */
    bool displayScreen(const std::string& processName) {
        std::lock_guard<std::mutex> lock(processesMutex);
        
        auto it = processes.find(processName);
        if (it == processes.end()) {
            std::cout << "Process " << processName << " not found.\n";
            return false;
        }

        auto process = it->second;
        
        std::cout << "\n===================================\n";
        std::cout << "Process: " << process->getPID() << "\n";
        std::cout << "Current Instruction Line: " << process->getCurrentLine() << "\n";
        std::cout << "Total Instructions: " << process->getTotalInstructions() << "\n";
        
        std::string state;
        switch(process->getState()) {
            case ProcessState::READY: state = "Ready"; break;
            case ProcessState::RUNNING: state = "Running"; break;
            case ProcessState::WAITING: state = "Waiting"; break;
            case ProcessState::FINISHED: state = "Finished"; break;
        }
        std::cout << "State: " << state << "\n";
        std::cout << "===================================\n\n";
        
        return true;
    }

    /**
     * Gets a process by name
     */
    std::shared_ptr<Process> getProcess(const std::string& processName) {
        std::lock_guard<std::mutex> lock(processesMutex);
        auto it = processes.find(processName);
        return (it != processes.end()) ? it->second : nullptr;
    }

    /**
     * Gets all processes (for ReportHandler to use)
     */
    std::vector<std::shared_ptr<Process>> getAllProcesses() {
        std::lock_guard<std::mutex> lock(processesMutex);
        std::vector<std::shared_ptr<Process>> result;
        for (const auto& pair : processes) {
            result.push_back(pair.second);
        }
        return result;
    }

    /**
     * Checks if a process exists
     */
    bool processExists(const std::string& processName) {
        std::lock_guard<std::mutex> lock(processesMutex);
        return processes.find(processName) != processes.end();
    }

    /**
     * Gets the number of processes
     */
    size_t getProcessCount() {
        std::lock_guard<std::mutex> lock(processesMutex);
        return processes.size();
    }

    /**
     * Removes a process from the registry
     */
    bool removeProcess(const std::string& processName) {
        std::lock_guard<std::mutex> lock(processesMutex);
        auto it = processes.find(processName);
        if (it != processes.end()) {
            processes.erase(it);
            return true;
        }
        return false;
    }
};
