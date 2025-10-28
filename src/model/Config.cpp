#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include "../view/misc.cpp"

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
    Config(const std::string &filepath)
    {
        this->time_between_instructions = -1;
        this->loadConfig(filepath);

        if (this->time_between_instructions == -1)
            // set to 100ms default
            this->time_between_instructions = 100;
    }

    int getNumCpus() const { return num_cpus; }
    int getQuantumCycles() const { return quantum_cycles; }
    int getBatchProcessFreq() const { return batch_process_freq; }
    int getMinInstructions() const { return min_instructions; }
    int getMaxInstructions() const { return max_instructions; }
    int getDelayPerExec() const { return delay_per_exec; }
    int getTimeBetweenInstructions() const { return time_between_instructions; }
    std::string getSchedulerAlgorithm() const { return scheduler_algorithm; }

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
    }
};