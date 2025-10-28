#include <iostream>
#include <vector>
#include "model\Config.cpp"
#include "model\CPU.cpp"
#include "model\readyqueue\ReadyQueue.cpp"
#include "view\CLI.cpp"
#include "handlers\CommandHandler.cpp"
#include <thread>
#include <atomic>

/**  TODO: TLDR there will be 2 threads:
 *   - Main thread: handles CLI input and command parsing
 *   - Scheduler thread: handles CPU scheduling and process execution
 */

class GreggyOS
{
    std::vector<CPU> cpus;
    std::atomic<bool> running;
    std::thread schedulerThread;
    CommandLineInterface cli;
    Config config;
    ReadyQueue readyQueue;

public:
    GreggyOS() : config("config.txt"), running(true)
    {
        this->readyQueue = ReadyQueue();
        this->cli = CommandLineInterface();

        // spawn CPUs based on config
        for (int i = 0; i < config.getNumCpus(); ++i)
        {
            cpus.emplace_back(i, config.getQuantumCycles(), this->readyQueue);
        }

        // spawn threads
        this->schedulerThread = std::thread(&GreggyOS::schedulerLoop, this, std::ref(this->running));
    }

    void schedulerLoop(std::atomic<bool> &running)
    {
        while (running)
        {
            this->handleNext();
            // Sleep or wait for a certain interval
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    void handleNext()
    {
        for (int i = 0; i < cpus.size(); ++i)
        {
            cpus[i].executeNext();
        }
    }
};

int main()
{
    GreggyOS os;
    os.handleNext();
    return 0;
}