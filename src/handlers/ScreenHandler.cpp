#pragma once
#include "../model/Process.cpp"
#include "../model/Config.cpp"
#include "GeneratorHandler.cpp"
#include "ScheduleHandler.cpp"
#include "ReportHandler.cpp"
#include <map>
#include <memory>
#include <mutex>
#include <iostream>

class ScreenHandler {
private:
    std::map<std::string, std::shared_ptr<Process>> processes;
    std::mutex processesMutex;
    Config* config;
    GeneratorHandler* generator;
    ScheduleHandler* scheduleHandler;
    ReportHandler* reportHandler;

public:
    ScreenHandler(Config* configPtr, 
                  GeneratorHandler* generatorPtr,
                  ScheduleHandler* scheduleHandlerPtr,
                  ReportHandler* reportHandlerPtr) 
        : config(configPtr), 
          generator(generatorPtr),
          scheduleHandler(scheduleHandlerPtr),
          reportHandler(reportHandlerPtr) {}

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
        
        // ADD TO SCHEDULER (Critical!)
        if (scheduleHandler) {
            scheduleHandler->addProcess(process);
        }
        
        // NOTIFY REPORT HANDLER (Important!)
        if (reportHandler) {
            reportHandler->recordProcessStart(process);
        }
        
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
        
        // Show current instruction
        std::cout << "\nCurrent instruction: " << process->getCurrentInstructionStr() << "\n";
        
        std::cout << "===================================\n";
        
        // Display process logs
        auto logs = process->getLogs();
        if (!logs.empty()) {
            std::cout << "\n--- Process Logs ---\n";
            for (const auto& log : logs) {
                std::cout << log << "\n";
            }
            std::cout << "--- End of Logs ---\n";
        } else {
            std::cout << "\n(No logs yet)\n";
        }
        
        std::cout << "\n";
        
        return true;
    }

    /**
     * Enters interactive process screen mode (screen -r <name>)
     * Allows commands: process-smi, exit
     */
    void enterInteractiveScreen(const std::string& processName) {
        auto process = getProcess(processName);
        if (!process) {
            std::cout << "Process " << processName << " not found.\n";
            return;
        }

        std::cout << "\n=========================================\n";
        std::cout << "Entered process screen: " << processName << "\n";
        std::cout << "Commands: process-smi, exit\n";
        std::cout << "=========================================\n\n";

        // Initial display
        displayProcessInfo(process);

        // Interactive loop
        while (true) {
            std::cout << "root@" << processName << ":~$ ";
            std::string input;
            std::getline(std::cin, input);

            // Trim whitespace
            size_t start = input.find_first_not_of(" \t");
            size_t end = input.find_last_not_of(" \t");
            if (start != std::string::npos && end != std::string::npos) {
                input = input.substr(start, end - start + 1);
            }

            if (input.empty()) {
                continue;
            }

            if (input == "exit") {
                std::cout << "\nExiting process screen...\n";
                break;
            }
            else if (input == "process-smi") {
                std::cout << "\n";
                displayProcessInfo(process);
            }
            else {
                std::cout << "Unknown command: " << input << "\n";
                std::cout << "Available commands: process-smi, exit\n";
            }
        }
    }

private:
    /**
     * Helper method to display process information (for process-smi command)
     */
    void displayProcessInfo(std::shared_ptr<Process> process) {
        std::cout << "===================================\n";
        std::cout << "Process: " << process->getPID() << "\n";
        std::cout << "Current Instruction Line: " << process->getCurrentLine() << " / " 
                  << process->getTotalInstructions() << "\n";
        
        std::string state;
        switch(process->getState()) {
            case ProcessState::READY: state = "Ready"; break;
            case ProcessState::RUNNING: state = "Running"; break;
            case ProcessState::WAITING: state = "Waiting"; break;
            case ProcessState::FINISHED: state = "Finished"; break;
        }
        std::cout << "State: " << state << "\n";
        
        // Show current instruction being executed
        std::cout << "\nCurrent instruction: " << process->getCurrentInstructionStr() << "\n";
        
        std::cout << "===================================\n";
        
        // Display recent logs (last 20 entries)
        auto logs = process->getLogs();
        if (!logs.empty()) {
            std::cout << "\n--- Process Output (last " 
                      << std::min(static_cast<size_t>(20), logs.size()) 
                      << " entries) ---\n";
            
            size_t startIdx = logs.size() > 20 ? logs.size() - 20 : 0;
            for (size_t i = startIdx; i < logs.size(); ++i) {
                std::cout << logs[i] << "\n";
            }
            std::cout << "--- End of Output ---\n";
        } else {
            std::cout << "\n(No output yet)\n";
        }
        
        std::cout << "\n";
    }

public:

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
