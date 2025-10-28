#include <iostream>
#include <vector>
#include <string>
#include "readyqueue/ReadyQueue.cpp"

class CPU
{
    std::vector<Process> processes;
    int coreID;
    std::vector<std::string> logs;
    // this only matters if we round robin is used
    int timeQuantum;
    int timeLeft;

public:
    CPU(int coreID, int timeQuantum, ReadyQueue &readyQueue)
    {
        this->coreID = coreID;
        this->processes = std::vector<Process>();
        this->timeQuantum = timeQuantum;
        this->timeLeft = timeQuantum;
    }

    /**
     * Adds a process to the CPU's process list
     */
    void addProcess(const Process &process)
    {
        processes.push_back(process);
    }

    /**
     * Executes the next process in the CPU's process list
     */
    void executeNext()
    {
        if (this->hasRemainingProcesses())
        {
            // pop
            Process currentProcess = processes.front();
            processes.erase(processes.begin());
            // Simulate process execution

            // TODO: parse process

            // TODO: switch case to call appropriate handlers

            // TODO: if scheduler is set to round robin, manage time quantum
            if (this->timeLeft > 0)
            {
                this->timeLeft--;
            }
            else
            {
                // reset time quantum, and send process back to the queue
                this->timeLeft = this->timeQuantum;
            }
        }
        else
        {
            std::cout << "No remaining processes to execute." << std::endl;
        }
    }

    /**
     * Checks if there are remaining processes to execute
     */
    bool hasRemainingProcesses() const
    {
        return !processes.empty();
    }

private:
    /**
     * Parses a process and performs the necessary actions
     */
    // TODO: change type as necessary
    void parseProcess(const Process &process)
    {
        // do some funky shell script parsing here
    }

    // TODO: add execution methods as necessary
};