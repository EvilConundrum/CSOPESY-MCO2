#include <string>
#include <fstream>
#include <algorithm>
#include <cctype>

class Config
{
    int num_cpus;
    std::string scheduler_algorithm;
    int quantum_cycles;
    int batch_process_freq;
    int min_instructions;
    int max_instructions;
    int delay_per_exec;

    // basically time between per instruction in milliseconds
    int time_between_instructions;

public:
    Config() {};

    int getNumCpus() const { return num_cpus; }
    int getQuantumCycles() const { return quantum_cycles; }
    int getBatchProcessFreq() const { return batch_process_freq; }
    int getMinInstructions() const { return min_instructions; }
    int getMaxInstructions() const { return max_instructions; }
    int getDelayPerExec() const { return delay_per_exec; }
    int getTimeBetweenInstructions() const { return time_between_instructions; }
    std::string getSchedulerAlgorithm() const { return scheduler_algorithm; }

    void getConfigFromFile(const std::string &filepath)
    {
        // TODO: read from config.txt and set the variables above
    }
};