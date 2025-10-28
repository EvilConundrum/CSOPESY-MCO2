#include <vector>
#include <queue>
#include <string>
#include "../Process.cpp"

class ReadyQueue
{
    std::queue<Process> processes;

public:
    ReadyQueue() {}

    /**
     * Adds a process to the ready queue
     */
    void addProcess(const Process &process)
    {
        processes.push(process);
    }


    /**
     * Pops the next process from the ready queue
     * @pre the queue is not empty
     */
    Process popProcess()
    {
        Process process = processes.front();
        processes.pop();
        return process;
    }
};