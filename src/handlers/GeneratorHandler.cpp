#include <memory>
#include <random>
#include <string>
#include "../model/Process.cpp"
#include "../model/Config.cpp"

class GeneratorHandler
{
    int processCounter;
    std::mt19937 rng;

public:
    GeneratorHandler()
    {
        this->processCounter = 0;
        rng.seed(std::chrono::system_clock::now().time_since_epoch().count());
    }

    /**
     * Generates a new process with random instructions based on config
     */
    std::shared_ptr<Process> generateProcess(const Config &config)
    {
        // Generate process name
        std::string processName = "process_" + std::to_string(processCounter++);

        // Determine number of instructions (random between min and max)
        std::uniform_int_distribution<int> dist(config.getMinInstructions(), config.getMaxInstructions());
        int numInstructions = dist(rng);

        // Create process
        auto process = std::make_shared<Process>(processName, numInstructions);

        // Generate instructions
        for (int i = 0; i < numInstructions; i++)
        {
            // TODO: Implement instruction generation logic
        }

        return process;
    }

    /**
     * Generates a batch of processes
     */
    std::vector<std::shared_ptr<Process>> generateBatch(const Config &config, int batchSize)
    {
        std::vector<std::shared_ptr<Process>> processes;
        for (int i = 0; i < batchSize; i++)
        {
            processes.push_back(generateProcess(config));
        }
        return processes;
    }

    /**
     * Resets the process counter
     */
    void resetCounter()
    {
        processCounter = 0;
    }

    /**
     * Gets the current process count
     */
    int getProcessCount() const
    {
        return processCounter;
    }

    /**
     * Generates a process with a specific name (for screen -s)
     */
    std::shared_ptr<Process> generateProcessWithName(const Config &config, const std::string& customName)
    {
        // Use custom name instead of auto-generated name
        
        int numInstructions = std::uniform_int_distribution<int>(
            config.getMinInstructions(),
            config.getMaxInstructions()
        )(rng);

        // Create process with custom name as PID
        auto process = std::make_shared<Process>(customName, numInstructions);

        // Generate instructions
        for (int i = 0; i < numInstructions; i++)
        {
            process->addInstruction(Instruction());
        }

        return process;
    }
};