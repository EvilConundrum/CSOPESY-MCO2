#include <iostream>
#include <vector>
#include <string>

class CPU
{
    std::vector<std::string> tasks;
    int coreID;
    std::vector<std::string> logs;

public:
    CPU(int coreID)
    {
        this->coreID = coreID;
        this->tasks = std::vector<std::string>();
    }

    /**
     * Adds a task to the CPU's task list
     */
    void addTask(const std::string &task)
    {
        tasks.push_back(task);
    }

    /**
     * Executes the next task in the CPU's task list
     */
    void executeNext()
    {
        if (this->hasRemainingTasks())
        {
            // pop
            std::string currentTask = tasks.front();
            tasks.erase(tasks.begin());
            // Simulate task execution

            // TODO: parse task

            // TODO: switch case to call appropriate handlers
        }
        else
        {
            std::cout << "No remaining tasks to execute." << std::endl;
        }
    }

    /**
     * Checks if there are remaining tasks to execute
     */
    bool hasRemainingTasks() const
    {
        return !tasks.empty();
    }

private:
    /**
     * Parses a task string and performs the necessary actions
     */
    // TODO: change type as necessary
    void parseTask(const std::string &task)
    {
        // do some funky shell script parsing here
    }

    // TODO: add execution methods as necessary
};