#include <string>
#include <vector>

class Process
{
    std::string PID;
    std::vector<std::string> instructions;

public:
    Process(std::string pid) : PID(pid)
    {
        this->instructions = std::vector<std::string>();
    }

    /**
     * Adds an instruction to the process
     */
    void addProcess(const std::string &instruction)
    {
        this->instructions.push_back(instruction);
    }

    /**
     * Pops the next instruction from the process
     */
    std::string popInstruction()
    {
        if (!instructions.empty())
        {
            std::string currentInstruction = instructions.front();
            instructions.erase(instructions.begin());
            return currentInstruction;
        }
        return "";
    }

    /**
     * Checks if the process has remaining instructions
     */
    bool hasInstructions() const
    {
        return !instructions.empty();
    }

    /**
     * Gets the PID of the process
     */
    std::string getPID() const
    {
        return PID;
    }
};