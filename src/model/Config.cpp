#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include "../view/misc.cpp"

class Config
{
    // MO1 configurations
    int num_cpus;
    std::string scheduler_algorithm;
    int quantum_cycles;
    int batch_process_freq;
    int min_instructions;
    int max_instructions;
    int delay_per_exec;
    int time_between_instructions;

    // MO2 configurations
    int max_overall_mem;
    int mem_per_frame;
    int min_mem_per_proc;
    int max_mem_per_proc;

public:
    Config(const std::string &filepath)
    {
        this->time_between_instructions = -1;
        this->loadConfig(filepath);

        if (this->time_between_instructions == -1)
        {
            this->time_between_instructions = 100; // set to 100ms default
        }
    }

    // MO1 getters
    int getNumCpus() const { return num_cpus; }
    std::string getSchedulerAlgorithm() const { return scheduler_algorithm; }
    int getQuantumCycles() const { return quantum_cycles; }
    int getBatchProcessFreq() const { return batch_process_freq; }
    int getMinInstructions() const { return min_instructions; }
    int getMaxInstructions() const { return max_instructions; }
    int getDelayPerExec() const { return delay_per_exec; }
    int getTimeBetweenInstructions() const { return time_between_instructions; }

    // MO2 getters
    int getMaxOverallMem() const { return max_overall_mem; }
    int getMemPerFrame() const { return mem_per_frame; }
    int getMinMemPerProc() const { return min_mem_per_proc; }
    int getMaxMemPerProc() const { return max_mem_per_proc; }

    void loadConfig(const std::string &filepath)
    {
        std::string line;

        std::ifstream ConfigFile(filepath);
        if (!ConfigFile)
            return;

        while (std::getline(ConfigFile, line))
        {
            this->parseLine(line);
        }

        ConfigFile.close();
    }

private:
    /**
     * Parses a line from the config file and
     * sets the corresponding configuration parameter.
     */
    void parseLine(std::string line)
    {
        std::vector<std::string> tokens = splitString(line, ' ');

        if (tokens.size() != 2)
            return;

        const std::string &key = tokens[0];
        const std::string &value = tokens[1];

        if (key == "num-cpu")
            this->num_cpus = std::stoi(value);
        else if (key == "schedule")
            this->scheduler_algorithm = value;
        else if (key == "quantum-cycles")
            this->quantum_cycles = std::stoi(value);
        else if (key == "batch-process-freq")
            this->batch_process_freq = std::stoi(value);
        else if (key == "min-ins")
            this->min_instructions = std::stoi(value);
        else if (key == "max-ins")
            this->max_instructions = std::stoi(value);
        else if (key == "delay-per-exec")
            this->delay_per_exec = std::stoi(value);
        else if (key == "time-between-instructions")
            this->time_between_instructions = std::stoi(value);
        else if (key == "max-overall-mem")
            this->max_overall_mem = std::stoi(value);
        else if (key == "mem-per-frame")
            this->mem_per_frame = std::stoi(value);
        else if (key == "min-mem-per-proc")
            this->min_mem_per_proc = std::stoi(value);
        else if (key == "max-mem-per-proc")
            this->max_mem_per_proc = std::stoi(value);
    }
};