#include <string>
#include <vector>

enum class InstructionType {
    PRINT,      // I/O operation - prints to console
    COMPUTE     // CPU computation operation
};

class Instruction
{
    InstructionType type;
    std::string command;
    std::vector<std::string> args;
    int lineNumber;

public:
    // Constructor with instruction type
    Instruction(InstructionType type, int lineNumber)
    {
        this->type = type;
        this->lineNumber = lineNumber;
        this->command = "";
        this->args = {};
    }

    // Constructor for string literals (non-const reference compatibility)
    Instruction(std::string command, std::vector<std::string> arguments)
    {
        this->command = command;
        this->args = arguments;
        this->lineNumber = 0;

        // Parse instruction type from command
        if (command == "print") {
            this->type = InstructionType::PRINT;
        } else {
            this->type = InstructionType::COMPUTE;
        }
    }

    // Default constructor
    Instruction() {
        this->type = InstructionType::COMPUTE;
        this->lineNumber = 0;
        this->command = "";
        this->args = {};
    }

    InstructionType getType() const 
    { 
        return type; 
    }

    std::string getCommand() const 
    { 
        return command; 
    }

    std::vector<std::string> getArgs() const 
    { 
        return args; 
    }

    int getLineNumber() const
    {
        return lineNumber;
    }

    /**
     * Executes the instruction based on its type
     */
    void execute() const
    {
        switch (type) {
            case InstructionType::PRINT:
                // Simulate I/O operation
                // std::cout << "Process " << processName << ": Executing PRINT at line " << lineNumber << std::endl;
                break;
            case InstructionType::COMPUTE:
                // Simulate CPU computation
                // No output for compute instructions
                break;
        }
    }

    /**
     * Checks if this is a valid instruction
     */
    bool isValid() const
    {
        return lineNumber >= 0;
    }
};
